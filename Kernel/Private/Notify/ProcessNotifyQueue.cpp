#include "Kernel/Private/Notify/ProcessNotifyQueue.h"

void FProcessNotifyQueue::OnProcessCreateNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FProcessNotifyInfo ProcessInfo{};
	ProcessInfo.CreatedAt.QuadPart = PsGetProcessCreateTimeQuadPart(Process);

	const bool bCreationSucceed = NT_SUCCESS(CreateInfo->CreationStatus);
	if (bCreationSucceed)
	{
		ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::Created;
	}
	else
	{
		if (CreateInfo->CreationStatus == STATUS_ACCESS_DENIED)
		{
			ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::CreationDenied;
		}
		else
		{
			ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::CreationFailed;
		}
	}

	ProcessInfo.ProcessId		  = HandleToULong(ProcessId);
	ProcessInfo.ParentProcessId   = HandleToULong(CreateInfo->ParentProcessId);
	ProcessInfo.CreatingProcessId = HandleToULong(CreateInfo->CreatingThreadId.UniqueProcess);
	ProcessInfo.CreatingThreadId  = HandleToULong(CreateInfo->CreatingThreadId.UniqueThread);
	ProcessInfo.CreationStatus	  = CreateInfo->CreationStatus;

	PCUNICODE_STRING CommandLine = CreateInfo->CommandLine;
	if (CommandLine != nullptr && CommandLine->Length > 0)
	{
		//uint16 CommandBufferSize = CoreMin(ProcessNotifyMaxCommandLine, CommandLine->Length);
		uint16 CommandBufferSize = CommandLine->Length;
		WCHAR* CommandBuffer = static_cast<WCHAR*>(ExAllocatePool2(POOL_FLAG_PAGED, CommandBufferSize + sizeof(WCHAR), 'NsPk'));
		if (CommandBuffer != nullptr)
		{
			RtlCopyMemory(CommandBuffer, CommandLine->Buffer, CommandBufferSize);
			CommandBuffer[CommandBufferSize / sizeof(WCHAR)] = L'\0';

			ProcessInfo.CommandLine = CommandBuffer;
			ProcessInfo.CommandLineChars = CommandBufferSize / sizeof(WCHAR);
		}
	}
	else
	{
		ProcessInfo.CommandLineChars = 0;
	}

	PCUNICODE_STRING ImageFileName = CreateInfo->ImageFileName;
	if (ImageFileName != nullptr && ImageFileName->Buffer != nullptr &&
		ImageFileName->Length >= sizeof(WCHAR) && ImageFileName->Length % sizeof(WCHAR) == 0)
	{
		const uint16 ImageChars = ImageFileName->Length / sizeof(WCHAR);
		uint16 BaseNameStart = 0;
		for (uint16 Index = 0; Index < ImageChars; ++Index)
		{
			if (ImageFileName->Buffer[Index] == L'\\' || ImageFileName->Buffer[Index] == L'/')
			{
				BaseNameStart = Index + 1;
			}
		}

		const uint16 BaseNameChars = ImageChars - BaseNameStart;
		const uint32 BaseNameBytes = static_cast<uint32>(BaseNameChars) * sizeof(WCHAR);
		if (BaseNameBytes != 0 && BaseNameBytes <= Argus::Notify::ProcessNotifyMaxImageFileNameBytes)
		{
			WCHAR* BaseName = static_cast<WCHAR*>(ExAllocatePool2(
				POOL_FLAG_PAGED, BaseNameBytes + sizeof(WCHAR), 'NmPk'));
			if (BaseName != nullptr)
			{
				RtlCopyMemory(BaseName, ImageFileName->Buffer + BaseNameStart, BaseNameBytes);
				BaseName[BaseNameChars] = L'\0';
				ProcessInfo.ImageFileName = BaseName;
				ProcessInfo.ImageFileNameChars = BaseNameChars;
			}
		}
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ProcessNotifyInfosLock);

	Argus::Notify::FProcessNotifyInfo EvictedInfo{};
	const bool bWasFull = this->ProcessNotifyInfos.IsFull();
	const bool bPushSucceed =
		this->ProcessNotifyInfos.PushDropOldestIfFull(MoveTemp(ProcessInfo), &EvictedInfo) != INDEX_NONE;
	if (!bPushSucceed || bWasFull)
	{
		++this->DroppedProcessNotifyCount;
	}

	ExReleasePushLockExclusive(&this->ProcessNotifyInfosLock);
	KeLeaveCriticalRegion();
}

void FProcessNotifyQueue::OnProcessExitNotify(PEPROCESS Process, HANDLE ProcessId)
{
	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FProcessNotifyInfo ProcessInfo{};
	KeQuerySystemTimePrecise(&ProcessInfo.ExitedAt);

	ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::Exited;
	ProcessInfo.ProcessId  = HandleToULong(ProcessId);
	ProcessInfo.ExitCode   = PsGetProcessExitStatus(Process);

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ProcessNotifyInfosLock);

	Argus::Notify::FProcessNotifyInfo EvictedInfo{};
	const bool bWasFull = this->ProcessNotifyInfos.IsFull();
	const bool bPushSucceed =
		this->ProcessNotifyInfos.PushDropOldestIfFull(MoveTemp(ProcessInfo), &EvictedInfo) != INDEX_NONE;
	if (!bPushSucceed || bWasFull)
	{
		++this->DroppedProcessNotifyCount;
	}

	ExReleasePushLockExclusive(&this->ProcessNotifyInfosLock);
	KeLeaveCriticalRegion();
}
