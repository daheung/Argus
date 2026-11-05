#include "Kernel/Private/DeviceControl/DeviceControl.h"
#include "Core/Public/DeviceControl/DeviceControlDefines.h"
#include "Core/Public/DeviceControl/Protocols/ProcessNotifyProtocol.h"
#include "Core/Public/DeviceControl/Protocols/ThreadNotifyProtocol.h"
#include "Core/Public/DeviceControl/Protocols/ImageNotifyProtorol.h"
#include "Core/Public/DeviceControl/Protocols/RegistryNotifyProtocol.h"
#include "Core/Public/DeviceControl/Protocols/ObjectOperationNotifyProtocol.h"
#include "Core/Public/DeviceControl/Protocols/RegisterAgentProtocol.h"
#include "Core/Public/DeviceControl/Protocols/RegisterProcessProtocol.h"
#include "Core/Public/DeviceControl/Protocols/RegisterIsolationProtocol.h"
#include "Core/Public/Memory/MemoryUtils.h"
#include "Kernel/Private/DriverContext.h"
#include "Kernel/Private/Logging.h"

/*
 * Evolution of the notification IOCTL protocols
 *
 * Process Notification
 *   Implementation:
 *     A variable-length command line was copied into a fixed-size notify slot.
 *     The executable base name was transmitted in a separate field.
 *   Problems:
 *     1. A long command line is truncated at the slot boundary. This also
 *        occurs when the command line contains a long executable path.
 *     2. A short command line leaves most of its reserved slot space unused,
 *        but the entire fixed-size slot is still transmitted.
 *
 * Thread Notification
 *   Implementation:
 *     Thread notifications contain no variable-length strings, so a
 *     fixed-size record remained suitable.
 *
 * Image Notification
 *   Implementation attempted:
 *     A variable-length string representation was considered for image paths.
 *   Problem:
 *     FString was still under development, and its stability had not been
 *     sufficiently verified for this use.
 *   Result:
 *     The queue stores a separately allocated path, but DeviceControl still
 *     transmits it through a fixed-size slot, where a long path is truncated.
 *
 * Registry Notification
 *   Design changes:
 *     1. After the required FString operations received initial implementation
 *        and verification, registry names were stored using FString.
 *     2. A versioned header and variable-length records were introduced so
 *        each record could carry its actual key-name and value-name lengths.
 *     3. Explicit lengths, offsets, and padding were added to identify record
 *        boundaries and preserve alignment within a batch.
 *     4. A record is removed from the queue only when it fits in the output
 *        buffer. The header reports NextRecordCount and NextRecordBytes so
 *        the Agent can request another batch or enlarge its buffer.
 *   Problem:
 *     Calculating the size of the next batch requires walking the remaining
 *     variable-length records while holding the queue lock. This work grows
 *     with the number of queued records and can delay the IOCTL response.
 *
 * ObjectOperation Notify
 *   Design changes:
 *     1. Object-operation records have a fixed size, so their remaining byte
 *        count does not require a walk through individual records.
 *     2. The Agent includes CurRecordCount and an upper bound for
 *        NextRecordCount in the request packet.
 *     3. The kernel reads an atomic snapshot of the queue count and applies
 *        the requested upper bound when writing NextRecordCount and
 *        NextRecordBytes to the response header.
 *   Contract:
 *     The next-batch count is a hint because the queue can change concurrently.
 *     The actual response count includes only records successfully removed
 *     from the queue and written to the output buffer.
 */

extern FDriverContext* GDriverContext;
extern "C" UCHAR* PsGetProcessImageFileName(PEPROCESS Process);

NTSTATUS DeviceControlGetProcessNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	const uint32 BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
	const uint32 InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

	auto* Buffer = static_cast<uint8*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	constexpr uint32 HeaderBytes = sizeof(Argus::DeviceControl::FProcessNotifyHeader);
	if (BufferLength < HeaderBytes)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}
	if (InputBufferLength < sizeof(Argus::DeviceControl::FProcessNotifyRP))
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}

	Argus::DeviceControl::FProcessNotifyRP RequestPacket{};
	{
		auto* TmpRp = static_cast<Argus::DeviceControl::FObjectOperationRP*>((void*)Buffer);
		if (TmpRp == nullptr)
		{
			return STATUS_INVALID_PARAMETER;
		}
		if (TmpRp->Version != Argus::DeviceControl::FProcessNotifyHeader::HeaderVersion)
		{
			return STATUS_REVISION_MISMATCH;
		}

		RequestPacket.Version		  = TmpRp->Version;
		RequestPacket.CurRecordCount  = TmpRp->CurRecordCount;
		RequestPacket.NextRecordCount = TmpRp->NextRecordCount;
	}

	FProcessNotifyQueue& ProcessQueue = Context->GetProcessNotifyQueue();
	FProcessNotifyQueue::TQueue& ProcessNotifyQueue = ProcessQueue.GetQueue();

	/**
	 * [BatchHeader]
	 * [Record 1] [CommandLine UTF - 16] [ImageFileName UTF - 16] [8-Bytes align padding]
	 * [Record 2] [CommandLine UTF - 16] [ImageFileName UTF - 16] [8-Bytes align padding]
	 */

	uint32 NumWrite = 0;
	uint64 NumWriteOffset = HeaderBytes;
	for (;; ++NumWrite)
	{
		ProcessQueue.Lock(ELockType::Exclusive);

		if (ProcessNotifyQueue.IsEmpty())
		{
			ProcessQueue.Unlock(ELockType::Exclusive);
			break;
		}

		Argus::Notify::FProcessNotifyInfo& PeekInfo = ProcessNotifyQueue.PeekFront();
		const uint64 CommandLineBytes	= static_cast<uint64>(PeekInfo.CommandLine.Len()) * sizeof(FString<>::ElementType);
		const uint64 ImageFileNameBytes = static_cast<uint64>(PeekInfo.ImageFileName.Len()) * sizeof(FString<>::ElementType);
		const uint64 RecordHeaderBytes  = sizeof(Argus::DeviceControl::FProcessNotifyRecord);

		const uint64 RawBytes	  = RecordHeaderBytes + CommandLineBytes + ImageFileNameBytes;
		const uint64 RecordBytes  = static_cast<uint64>(AlignUp(RawBytes, Argus::DeviceControl::FProcessNotifyRecord::RecordAlignment));
		const uint64 PaddingBytes = static_cast<uint64>(RecordBytes - RawBytes);

		const uint32 BufferSlack = static_cast<uint32>(BufferLength - NumWriteOffset);
		if (BufferSlack < RecordBytes)
		{
			ProcessQueue.Unlock(ELockType::Exclusive);
			break;
		}

		Argus::Notify::FProcessNotifyInfo Info;
		const bool bPopSucceed = ProcessNotifyQueue.TryPop(Info);

		ProcessQueue.Unlock(ELockType::Exclusive);

		if (!bPopSucceed)
		{
			break;
		}

		RtlZeroMemory(Buffer + NumWriteOffset, RecordBytes);

		Argus::DeviceControl::FProcessNotifyRecord Record{};
		Record.RecordHeaderBytes = sizeof(Argus::DeviceControl::FProcessNotifyRecord);
		Record.NotifyType		 = Info.NotifyType;
		Record.PolicyRole		 = Info.PolicyRole;
		Record.PolicyStatus		 = Info.PolicyStatus;
		Record.CreatedAt		 = Info.CreatedAt.QuadPart;
		Record.ExitedAt			 = Info.ExitedAt.QuadPart;
		Record.ProcessId		 = Info.ProcessId;
		Record.ParentProcessId	 = Info.ParentProcessId;
		Record.CreatingThreadId  = Info.CreatingThreadId;
		Record.CreatingProcessId = Info.CreatingProcessId;
		Record.ExitCode			 = Info.ExitCode;
		Record.CreationStatus	 = Info.CreationStatus;
		
		Record.CommandLineOffset = NumWriteOffset + Record.RecordHeaderBytes;
		Record.CommandLineBytes	 = CommandLineBytes;
		Record.ImageFileNameOffset = Record.CommandLineOffset + Record.CommandLineBytes;
		Record.ImageFileNameBytes  = ImageFileNameBytes;
		Record.PaddingBytes = PaddingBytes;

		RtlCopyMemory(&Buffer[NumWriteOffset], &Record, Record.RecordHeaderBytes);

		if (CommandLineBytes != 0)
		{
			RtlCopyMemory(&Buffer[Record.CommandLineOffset], Info.CommandLine.GetCharArray().GetData(), Record.CommandLineBytes);
		}
		if (ImageFileNameBytes != 0)
		{
			RtlCopyMemory(&Buffer[Record.ImageFileNameOffset], Info.ImageFileName.GetCharArray().GetData(), Record.ImageFileNameBytes);
		}

		NumWriteOffset += RecordBytes;
	}

	ProcessQueue.Lock(ELockType::Shared);

	uint32 NextRecordCount = 0;
	uint64 NextRecordBytes = 0;
	for (auto Iterator = ProcessNotifyQueue.Begin(); Iterator != ProcessNotifyQueue.End(); ++Iterator)
	{
		Argus::Notify::FProcessNotifyInfo& Info(*Iterator);
		const uint64 CommandLineBytes	= static_cast<uint64>(Info.CommandLine.Len()) * sizeof(FString<>::ElementType);
		const uint64 ImageFileNameBytes = static_cast<uint64>(Info.ImageFileName.Len()) * sizeof(FString<>::ElementType);
		const uint64 RecordHeaderBytes	= sizeof(Argus::DeviceControl::FProcessNotifyRecord);

		const uint64 RawBytes	 = RecordHeaderBytes + CommandLineBytes + ImageFileNameBytes;
		const uint64 RecordBytes = static_cast<uint64>(AlignUp(RawBytes, Argus::DeviceControl::FProcessNotifyRecord::RecordAlignment));

		NextRecordCount += 1;
		NextRecordBytes += RecordBytes;
	}

	ProcessQueue.Unlock(ELockType::Shared);

	Argus::DeviceControl::FProcessNotifyHeader Header{};
	Header.Version		   = Argus::DeviceControl::FProcessNotifyHeader::HeaderVersion;
	Header.HeaderBytes	   = HeaderBytes;
	Header.TotalCount	   = NumWrite;
	Header.TotalBytes	   = NumWriteOffset;
	Header.DropCount	   = ProcessQueue.GetDroppedCount();
	Header.NextRecordCount = NextRecordCount;
	Header.NextRecordBytes = NextRecordBytes;
	RtlCopyMemory(Buffer, &Header, sizeof(Argus::DeviceControl::FProcessNotifyHeader));

	Irp->IoStatus.Information = static_cast<ULONG_PTR>(NumWriteOffset);
	return STATUS_SUCCESS;
}

NTSTATUS DeviceControlGetThreadNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	const uint32 BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

	constexpr uint32 RecordBytes = sizeof(Argus::DeviceControl::FThreadNotifyRecord);
	if (BufferLength < RecordBytes)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}
	if (BufferLength % RecordBytes != 0)
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}

	auto* Buffer = static_cast<uint8*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	const uint32 BufferCount = BufferLength / RecordBytes;
	const uint32 MaxCount = CoreMin(BufferCount, static_cast<uint32>(FThreadNotifyQueue::ThreadNotifyMaxInfos));
	FThreadNotifyQueue& ThreadQueue = Context->GetThreadNotifyQueue();

	uint32 NumWrite = 0;
	for (; NumWrite < MaxCount; ++NumWrite)
	{
		Argus::Notify::FThreadNotifyInfo Info;
		if (!ThreadQueue.TryPop(Info))
		{
			break;
		}

		Argus::DeviceControl::FThreadNotifyRecord Record{};
		Record.NotifyType = Info.NotifyType;
		Record.CreatedAt  = Info.CreatedAt.QuadPart;
		Record.ExitedAt   = Info.ExitedAt.QuadPart;
		Record.ProcessId  = Info.ProcessId;
		Record.ThreadId	  = Info.ThreadId;

		const size_t RecordStride = static_cast<size_t>(NumWrite) * RecordBytes;
		RtlCopyMemory(Buffer + RecordStride, &Record, RecordBytes);
	}

	Irp->IoStatus.Information = static_cast<ULONG_PTR>(NumWrite) * RecordBytes;
	return STATUS_SUCCESS;
}

NTSTATUS DeviceControlGetImageNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	const uint32 BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

	constexpr uint32 SlotBytes = sizeof(Argus::DeviceControl::FImageNotifySlot);
	if (BufferLength < SlotBytes)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}
	if (BufferLength % SlotBytes != 0)
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}

	auto* Buffer = static_cast<uint8*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	const uint32 BufferCount = BufferLength / SlotBytes;

	FImageNotifyQueue& ImageQueue = Context->GetImageNotifyQueue();

	uint32 NumWrite = 0;
	for (uint32 Index = 0; Index < BufferCount; ++Index, ++NumWrite)
	{
		Argus::Notify::FImageNotifyInfo Info;
		if (!ImageQueue.TryPop(Info))
		{
			break;
		}

		Argus::DeviceControl::FImageNotifySlot Slot;
		RtlZeroMemory(&Slot, sizeof(Slot));

		Slot.Record.LoadedAt  = Info.LoadedAt.QuadPart;
		Slot.Record.ProcessId = Info.ProcessId;
		Slot.Record.ImageSize = Info.ImageSize;
		Slot.Record.ImageBaseAddress = Info.ImageBaseAddress;
		
		const uint32 ImageFileNameBytes = static_cast<uint32>(Info.ImageFileNameChars) * sizeof(WCHAR);
		if (Info.ImageFileName != nullptr)
		{
			const uint32 CommandBufferBytes = CoreMin(ImageFileNameBytes, sizeof(Slot.ImageFileName) - sizeof(WCHAR));
			Slot.Record.ImageFileNameBytes = CommandBufferBytes;

			if (CommandBufferBytes != 0)
			{
				RtlCopyMemory(Slot.ImageFileName, Info.ImageFileName, CommandBufferBytes);
			}
		}

		const size_t SlotStride = static_cast<size_t>(Index) * SlotBytes;
		RtlCopyMemory(Buffer + SlotStride, &Slot, SlotBytes);
	}

	Irp->IoStatus.Information = static_cast<ULONG_PTR>(NumWrite) * SlotBytes;
	return STATUS_SUCCESS;
}

NTSTATUS DeviceControlGetRegistryNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	const uint32 BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

	auto* Buffer = static_cast<uint8*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	constexpr uint32 HeaderBytes = sizeof(Argus::DeviceControl::FRegistryNotifyHeader);
	if (BufferLength < HeaderBytes)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	FRegistryNotifyQueue& RegistryQueue = Context->GetRegistryNotifyQueue();
	FRegistryNotifyQueue::TQueue& RegistryNotifyQueue = RegistryQueue.GetQueue();

	/**
	 * [BatchHeader]
	 * [Record 1] [KeyName UTF - 16] [ValueName UTF - 16] [8-Bytes align padding]
	 * [Record 2] [KeyName UTF - 16] [ValueName UTF - 16] [8-Bytes align padding]
	 */

	uint64 NumWriteStartOffset = HeaderBytes;
	for (;;)
	{
		RegistryQueue.Lock(ELockType::Exclusive);

		if (RegistryNotifyQueue.IsEmpty())
		{
			RegistryQueue.Unlock(ELockType::Exclusive);
			break;
		}

		Argus::Notify::FRegistryNotifyInfo& PeekInfo = RegistryNotifyQueue.PeekFront();
		const uint64 KeyNameBytes	= static_cast<uint64>(PeekInfo.KeyName.Len()) * sizeof(FString<>::ElementType);
		const uint64 ValueNameBytes = static_cast<uint64>(PeekInfo.ValueName.Len()) * sizeof(FString<>::ElementType);
		const uint64 RecordHeaderBytes = sizeof(Argus::DeviceControl::FRegistryNotifyRecord);

		const uint64 RawBytes	  = RecordHeaderBytes + KeyNameBytes + ValueNameBytes;
		const uint64 RecordBytes  = static_cast<uint64>(AlignUp(RawBytes, Argus::DeviceControl::FRegistryNotifyRecord::RecordAlignment));
		const uint64 PaddingBytes = static_cast<uint64>(RecordBytes - RawBytes);

		const uint32 BufferSlack = static_cast<uint32>(BufferLength - NumWriteStartOffset);
		
		if (BufferSlack < RecordBytes)
		{
			RegistryQueue.Unlock(ELockType::Exclusive);
			break;
		}

		Argus::Notify::FRegistryNotifyInfo Info;
		const bool bPopSucceed = RegistryQueue.TryPop(Info);

		RegistryQueue.Unlock(ELockType::Exclusive);

		if (!bPopSucceed)
		{
			break;
		}
		
		RtlZeroMemory(Buffer + NumWriteStartOffset, RecordBytes);

		Argus::DeviceControl::FRegistryNotifyRecord Record{};
		Record.RecordHeaderBytes = sizeof(Argus::DeviceControl::FRegistryNotifyRecord);
		// [Record 1] + [KeyName UTF - 16] + [ValueName UTF - 16] + [8-Bytes align padding]
		Record.RecordBytes = RecordBytes;

		Record.RequesterRoles  = Info.RequesterRoles;
		Record.OperationId	   = Info.OperationId;
		Record.OperationType   = Info.OperationType;
		Record.Decision		   = Info.Decision;
		Record.Phase		   = Info.Phase;
		Record.KeyNameStatus   = Info.KeyNameStatus;
		Record.ValueNameStatus = Info.ValueNameStatus;

		Record.OccurredAt = Info.OccurredAt.QuadPart;
		Record.Status	  = Info.Status;
		Record.ProcessId  = Info.ProcessId;
		Record.ThreadId   = Info.ThreadId;
		Record.IsolationFlags = Info.IsolationFlags;

		Record.KeyNameOffset   = NumWriteStartOffset + Record.RecordHeaderBytes;
		Record.KeyNameBytes	   = KeyNameBytes;
		Record.ValueNameOffset = Record.KeyNameOffset + Record.KeyNameBytes;
		Record.ValueNameBytes  = ValueNameBytes;
		Record.PaddingBytes	   = PaddingBytes;

		RtlCopyMemory(&Buffer[NumWriteStartOffset], &Record, Record.RecordHeaderBytes);

		if (KeyNameBytes != 0)
		{
			RtlCopyMemory(&Buffer[Record.KeyNameOffset], Info.KeyName.GetCharArray().GetData(), Record.KeyNameBytes);
		}
		if (ValueNameBytes != 0)
		{
			RtlCopyMemory(&Buffer[Record.ValueNameOffset], Info.ValueName.GetCharArray().GetData(), Record.ValueNameBytes);
		}

		NumWriteStartOffset += RecordBytes;
	}

	RegistryQueue.Lock(ELockType::Shared);

	uint32 NextRecordCount = 0;
	uint64 NextRecordBytes = 0;
	for (auto Iterator = RegistryNotifyQueue.Begin(); Iterator != RegistryNotifyQueue.End(); ++Iterator)
	{
		Argus::Notify::FRegistryNotifyInfo& Info(*Iterator);
		const uint64 KeyNameBytes	= static_cast<uint64>(Info.KeyName.Len()) * sizeof(FString<>::ElementType);
		const uint64 ValueNameBytes = static_cast<uint64>(Info.ValueName.Len()) * sizeof(FString<>::ElementType);
		const uint64 RecordHeaderBytes = sizeof(Argus::DeviceControl::FRegistryNotifyRecord);

		const uint64 RawBytes = RecordHeaderBytes + KeyNameBytes + ValueNameBytes;
		const uint64 RecordBytes = static_cast<uint64>(AlignUp(RawBytes, Argus::DeviceControl::FRegistryNotifyRecord::RecordAlignment));

		NextRecordCount += 1;
		NextRecordBytes += RecordBytes;
	}
	
	RegistryQueue.Unlock(ELockType::Shared);

	Argus::DeviceControl::FRegistryNotifyHeader Header{};
	Header.Version = Argus::DeviceControl::FRegistryNotifyHeader::HeaderVerion;
	Header.HeaderBytes = sizeof(Argus::DeviceControl::FRegistryNotifyHeader);
	Header.TotalBytes = NumWriteStartOffset;
	Header.DropCount = RegistryQueue.GetDroppedCount();
	Header.NextRecordCount = NextRecordCount;
	Header.NextRecordBytes = NextRecordBytes;
	RtlCopyMemory(Buffer, &Header, sizeof(Argus::DeviceControl::FRegistryNotifyHeader));

	Irp->IoStatus.Information = static_cast<ULONG_PTR>(NumWriteStartOffset);
	return STATUS_SUCCESS;
}

NTSTATUS DeviceControlGetObjectOperationNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	const uint32 BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
	const uint32 InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

	auto* Buffer = static_cast<uint8*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	constexpr uint32 HeaderBytes = sizeof(Argus::DeviceControl::FObjectOperationNotifyHeader);
	if (BufferLength < HeaderBytes)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}
	if (InputBufferLength < sizeof(Argus::DeviceControl::FObjectOperationRP))
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}

	Argus::DeviceControl::FObjectOperationRP RequestPacket{};
	{
		auto* TmpRp = static_cast<Argus::DeviceControl::FObjectOperationRP*>((void*)Buffer);
		if (TmpRp == nullptr)
		{
			return STATUS_INVALID_PARAMETER;
		}

		RequestPacket.Version = TmpRp->Version;
		RequestPacket.CurRecordCount = TmpRp->CurRecordCount;
		RequestPacket.NextRecordCount = TmpRp->NextRecordCount;
		if (RequestPacket.Version != Argus::DeviceControl::FObjectOperationNotifyHeader::HeaderVersion)
		{
			return STATUS_REVISION_MISMATCH;
		}
	}
	
	constexpr uint32 RecordBytes = sizeof(Argus::DeviceControl::FObjectOperationNotifyRecord);
	const uint32 BufferRecordCount = CoreMin((BufferLength - HeaderBytes) / RecordBytes, RequestPacket.CurRecordCount);

	/**
	 * [BatchHeader]
	 * [Record 1] [Record 2]
	 */

	FObjectOperationNotifyQueue& ObjectOperationQueue = Context->GetObjectOperationNotifyQueue();
	FObjectOperationNotifyQueue::TQueue& ObjectOperationNotifyQueue = ObjectOperationQueue.GetQueue();

	uint64 WriteNum = 0;
	for (; WriteNum < BufferRecordCount; ++WriteNum)
	{
		ObjectOperationQueue.Lock(ELockType::Exclusive);
		
		Argus::Notify::FObjectOperationNotifyInfo Info;
		const bool bPopSucceed = ObjectOperationNotifyQueue.TryPop(Info);

		ObjectOperationQueue.Unlock(ELockType::Exclusive);

		if (!bPopSucceed)
		{
			break;
		}

		Argus::DeviceControl::FObjectOperationNotifyRecord Record{};
		Record.PreOperationAt			 = Info.PreOperationAt.QuadPart;
		Record.PostOperationAt			 = Info.PostOperationAt.QuadPart;
		Record.OperationType			 = Info.OperationType;
		Record.TargetType				 = Info.TargetType;
		Record.RequesterProcessId		 = Info.RequesterProcessId;
		Record.RequesterThreadId		 = Info.RequesterThreadId;
		Record.HandleOwnerProcessId		 = Info.HandleOwnerProcessId;
		Record.SourceProcessId			 = Info.SourceProcessId;
		Record.TargetProcessId			 = Info.TargetProcessId;
		Record.TargetThreadId			 = Info.TargetThreadId;
		Record.OriginalDesiredAccess	 = Info.OriginalDesiredAccess;
		Record.DesiredAccessBeforePolicy = Info.DesiredAccessBeforePolicy;
		Record.DesiredAccessAfterPolicy  = Info.DesiredAccessAfterPolicy;
		Record.GrantedAccess			 = Info.GrantedAccess;
		Record.ReturnStatus				 = Info.ReturnStatus;

		const uint64 RecordOffset = HeaderBytes + WriteNum * RecordBytes;
		RtlCopyMemory(&Buffer[RecordOffset], &Record, RecordBytes);
	}

	// Q. Why does DeviceControlGetObjectOperationNotify calculate the next batch
	// size without locking, unlike DeviceControlGetRegistryNotify?
	//
	// A. Object-operation records have a fixed size, so the next batch size can
	// be calculated using only an atomic snapshot of the queue count, without
	// accessing individual records.
	//
	// The snapshot is used solely as a sizing hint. Concurrent queue
	// modifications, including drop-oldest operations, may change the actual
	// record count. This is safe because records are accessed only through
	// synchronized TryPop() operations, and only successfully popped records
	// are serialized.
	//
	// In contrast, DeviceControlGetRegistryNotify calculates the remaining
	// byte count by iterating over queued records. A shared lock is required
	// to prevent concurrent modifications from invalidating iterators or
	// record contents during traversal.
	const uint32 QueueNum = static_cast<uint32>(ObjectOperationNotifyQueue.Num());
	uint32 NextRecordCount = CoreMin(QueueNum, RequestPacket.NextRecordCount);
	uint64 NextRecordBytes = NextRecordCount * RecordBytes;

	Argus::DeviceControl::FObjectOperationNotifyHeader Header{};
	Header.Version = Argus::DeviceControl::FObjectOperationNotifyHeader::HeaderVersion;
	Header.HeaderBytes = HeaderBytes;
	Header.TotalCount = WriteNum;
	Header.TotalBytes = HeaderBytes + WriteNum * RecordBytes;
	Header.DropCount = ObjectOperationQueue.GetDroppedCount();
	Header.NextRecordCount = NextRecordCount;
	Header.NextRecordBytes = NextRecordBytes;
	RtlCopyMemory(Buffer, &Header, HeaderBytes);

	Irp->IoStatus.Information = HeaderBytes + WriteNum * RecordBytes;
	return STATUS_SUCCESS;
}

NTSTATUS DeviceControlRegisterAgent(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	const uint32 BufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
	if (BufferLength != sizeof(Argus::DeviceControl::FRegisterAgent))
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}

	const auto* Buffer = static_cast<Argus::DeviceControl::FRegisterAgent*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	PEPROCESS Process = IoGetRequestorProcess(Irp);
	if (Process == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	FArgusAgentProcess AgentProcess{};
	AgentProcess.Process = Process;
	AgentProcess.MajorVersion = Buffer->MajorVersion;
	AgentProcess.MinorVersion = Buffer->MinorVersion;
	AgentProcess.PatchVersion = Buffer->PatchVersion;

	// A registered process cannot be replaced while it is active. Caller identity
	// still needs to be bound to the protected Agent installation at deployment.
	const NTSTATUS Status = Context->GetAgentState().RegisterAgentProcess(AgentProcess);
	if (NT_SUCCESS(Status))
	{
		Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "INFO", "process.register",
			"role=agent image=\"%.15s\" pid=%lu version=%u.%u.%u\n",
			reinterpret_cast<const char*>(PsGetProcessImageFileName(Process)),
			HandleToULong(PsGetProcessId(Process)),
			AgentProcess.MajorVersion,
			AgentProcess.MinorVersion,
			AgentProcess.PatchVersion
		);
	}
	return Status;
}

NTSTATUS DeviceControlRegisterProcess(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}
	if (!Context->GetAgentState().IsValidAgent())
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(Argus::DeviceControl::FInRegisterProcess))
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}
	if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(Argus::DeviceControl::FOutRegisterProcess))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	auto* Buffer = static_cast<Argus::DeviceControl::FInRegisterProcess*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}
	if (Buffer->CreatedAt <= 0)
	{
		return STATUS_INVALID_PARAMETER;
	}

	const HANDLE ProtectPid = static_cast<HANDLE>(ULongToHandle(Buffer->ProcessId));
	const int64 ExpectedCreatedAt = Buffer->CreatedAt;

	auto RegisterStatus = Argus::DeviceControl::ERegisterProcessStatus::Success;

	PEPROCESS RequestorProcess = IoGetRequestorProcess(Irp);
	if (RequestorProcess == nullptr || !Context->GetAgentState().IsAgentProcess(RequestorProcess))
	{
		RegisterStatus = Argus::DeviceControl::ERegisterProcessStatus::AccessDenied;
	}
	else
	{
		PEPROCESS Process = nullptr;
		const NTSTATUS LookupStatus = PsLookupProcessByProcessId(ProtectPid, &Process);
		if (!NT_SUCCESS(LookupStatus))
		{
			RegisterStatus = Argus::DeviceControl::ERegisterProcessStatus::ProcessNotFound;
		}
		else
		{
			if (PsGetProcessCreateTimeQuadPart(Process) != ExpectedCreatedAt)
			{
				RegisterStatus = Argus::DeviceControl::ERegisterProcessStatus::ProcessNotFound;
			}
			else
			{
				bool bIsExistRegistered = false;
				if (!Context->GetProtectedProcessRegistry().RegisterProcess(Process, &bIsExistRegistered))
				{
					RegisterStatus = bIsExistRegistered
						? Argus::DeviceControl::ERegisterProcessStatus::AlreadyRegistered
						: Argus::DeviceControl::ERegisterProcessStatus::AllocationFailed;
				}
				else
				{
					Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "INFO", "process.register",
						"role=protected image=\"%.15s\" pid=%lu\n",
						reinterpret_cast<const char*>(PsGetProcessImageFileName(Process)),
						HandleToULong(PsGetProcessId(Process))
					);
				}
			}
			ObDereferenceObject(Process);
		}
	}

	const Argus::DeviceControl::FOutRegisterProcess Out{ RegisterStatus };
	RtlCopyMemory(Buffer, &Out, sizeof(Out));

	Irp->IoStatus.Information = sizeof(Out);
	return STATUS_SUCCESS;
}

NTSTATUS DeviceControlRegisterIsolationProcess(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	Irp->IoStatus.Information = 0;

	if (Context == nullptr)
	{
		return STATUS_DEVICE_NOT_READY;
	}

	const auto* IrpSp = IoGetCurrentIrpStackLocation(Irp);
	if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(Argus::DeviceControl::FInRegisterIsolationProcess))
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}
	if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(Argus::DeviceControl::FOutRegisterIsolationProcess))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	auto* Buffer = static_cast<Argus::DeviceControl::FInRegisterIsolationProcess*>(Irp->AssociatedIrp.SystemBuffer);
	if (Buffer == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}
	if (Buffer->CreatedAt <= 0)
	{
		return STATUS_INVALID_PARAMETER;
	}

	const HANDLE ProtectPid = static_cast<HANDLE>(ULongToHandle(Buffer->ProcessId));
	const int64 ExpectedCreatedAt = Buffer->CreatedAt;

	auto RegisterStatus = Argus::DeviceControl::ERegisterIsolationProcessStatus::Success;

	PEPROCESS RequestorProcess = IoGetRequestorProcess(Irp);
	if (RequestorProcess == nullptr || !Context->GetAgentState().IsAgentProcess(RequestorProcess))
	{
		RegisterStatus = Argus::DeviceControl::ERegisterIsolationProcessStatus::AccessDenied;
	}
	else
	{
		PEPROCESS Process = nullptr;
		const NTSTATUS LookupStatus = PsLookupProcessByProcessId(ProtectPid, &Process);
		if (!NT_SUCCESS(LookupStatus))
		{
			RegisterStatus = Argus::DeviceControl::ERegisterIsolationProcessStatus::ProcessNotFound;
		}
		else
		{
			if (PsGetProcessCreateTimeQuadPart(Process) != ExpectedCreatedAt)
			{
				RegisterStatus = Argus::DeviceControl::ERegisterIsolationProcessStatus::ProcessNotFound;
			}
			else
			{
				EIsolationFlags IsolationFlags{ static_cast<uint32>(Buffer->Flags) };
				bool bIsExistRegistered = false;
				if (!Context->GetIsolationProcessRegistry().RegisterProcess(Process, IsolationFlags , &bIsExistRegistered))
				{
					RegisterStatus = bIsExistRegistered
						? Argus::DeviceControl::ERegisterIsolationProcessStatus::AlreadyRegistered
						: Argus::DeviceControl::ERegisterIsolationProcessStatus::AllocationFailed;
				}
				else
				{
					Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "INFO", "process.register",
						"role=isolation image=\"%.15s\" pid=%lu flags=0x%X\n",
						reinterpret_cast<const char*>(PsGetProcessImageFileName(Process)),
						HandleToULong(PsGetProcessId(Process)),
						static_cast<uint32>(IsolationFlags)
					);
				}
			}
			ObDereferenceObject(Process);
		}
	}

	const Argus::DeviceControl::FOutRegisterIsolationProcess Out{ RegisterStatus };
	RtlCopyMemory(Buffer, &Out, sizeof(Out));

	Irp->IoStatus.Information = sizeof(Out);
	return STATUS_SUCCESS;
}

namespace Driver
{
	NTSTATUS ArgusDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
	{
		auto IrpSp = IoGetCurrentIrpStackLocation(Irp);

		Irp->IoStatus.Information = 0;
		NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
		switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
		{
		case Argus::DeviceControl::IoctlCodeGetProcessNotify:
			Status = DeviceControlGetProcessNotify(GDriverContext, DeviceObject, Irp);
			break;
		case Argus::DeviceControl::IoctlCodeGetThreadNotify:
			Status = DeviceControlGetThreadNotify(GDriverContext, DeviceObject, Irp);
			break;
		case Argus::DeviceControl::IoctlCodeGetImageNotify:
			Status = DeviceControlGetImageNotify(GDriverContext, DeviceObject, Irp);
			break;
		case Argus::DeviceControl::IoctlCodeGetRegistryNotify:
			Status = DeviceControlGetRegistryNotify(GDriverContext, DeviceObject, Irp);
			break;
		case Argus::DeviceControl::IoctlCodeGetObjectOperationNotify:
			Status = DeviceControlGetObjectOperationNotify(GDriverContext, DeviceObject, Irp);
			break;
		case Argus::DeviceControl::IoctlCodeRegisterAgent:
			Status = DeviceControlRegisterAgent(GDriverContext, DeviceObject, Irp);
			break;
		case Argus::DeviceControl::IoctlCodeRegisterProcess:
			Status = DeviceControlRegisterProcess(GDriverContext, DeviceObject, Irp);
			break;
		case Argus::DeviceControl::IoctlCodeRegisterIsocationProcess:
			Status = DeviceControlRegisterIsolationProcess(GDriverContext, DeviceObject, Irp);
			break;
		default:
			break;
		}

		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return Status;
	}
}
