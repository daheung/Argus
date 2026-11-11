#include "Kernel/Private/Minifilter/Operation/CreateOperation.h"
#include "Kernel/Private/DriverContext.h"
#include "Kernel/Private/Minifilter/Minifilter.h"

namespace
{
	constexpr uint32 CreateContextTag = 'rCFk';
	constexpr uint32 VolumeNameTag = 'loVk';

	void ReleaseCreateContext(Argus::Minifilter::FCreateOperationDescriptor* Context) noexcept
	{
		if (Context != nullptr)
		{
			DestructItem(Context);
			ExFreePoolWithTag(Context, CreateContextTag);
		}
	}

	void CaptureVolumeName(PFLT_VOLUME Volume, FString<>& OutName,
		Argus::Minifilter::EOperationStringStatus& OutStatus) noexcept
	{
		using Argus::Minifilter::EOperationStringStatus;
		if (Volume == nullptr)
		{
			OutStatus = EOperationStringStatus::Unavailable;
			return;
		}

		ULONG RequiredBytes = 0;
		const NTSTATUS SizeStatus = FltGetVolumeName(Volume, nullptr, &RequiredBytes);
		if (SizeStatus != STATUS_BUFFER_TOO_SMALL || RequiredBytes == 0 ||
			RequiredBytes > TNumericLimits<uint16>::Max())
		{
			OutStatus = EOperationStringStatus::Unavailable;
			return;
		}

		UNICODE_STRING Name{};
		Name.MaximumLength = static_cast<uint16>(RequiredBytes);
		Name.Buffer = static_cast<PWCH>(ExAllocatePool2(POOL_FLAG_PAGED, RequiredBytes, VolumeNameTag));
		if (Name.Buffer == nullptr)
		{
			OutStatus = EOperationStringStatus::AllocationFailed;
			return;
		}

		const NTSTATUS NameStatus = FltGetVolumeName(Volume, &Name, nullptr);
		if (NT_SUCCESS(NameStatus))
		{
			OutStatus = OutName.InitializeWithLength(Name.Buffer,
				static_cast<int32>(Name.Length / sizeof(WCHAR)))
				? EOperationStringStatus::Captured
				: EOperationStringStatus::AllocationFailed;
		}
		else
		{
			OutStatus = EOperationStringStatus::Unavailable;
		}
		ExFreePoolWithTag(Name.Buffer, VolumeNameTag);
	}

	void CaptureFilePathName(PFLT_CALLBACK_DATA CallbackData, FLT_FILE_NAME_OPTIONS NameOptions,
		FString<>& OutName, Argus::Minifilter::EOperationStringStatus& OutStatus) noexcept
	{
		using Argus::Minifilter::EOperationStringStatus;
		PFLT_FILE_NAME_INFORMATION NameInformation = nullptr;
		const NTSTATUS Status = FltGetFileNameInformation(CallbackData, NameOptions, &NameInformation);
		if (NT_SUCCESS(Status) && NameInformation != nullptr)
		{
			if (NameInformation->Name.Buffer != nullptr)
			{
				OutStatus = OutName.InitializeWithLength(NameInformation->Name.Buffer,
					static_cast<int32>(NameInformation->Name.Length / sizeof(WCHAR)))
					? EOperationStringStatus::Captured
					: EOperationStringStatus::AllocationFailed;
			}
			else
			{
				OutStatus = EOperationStringStatus::Unavailable;
			}
			FltReleaseFileNameInformation(NameInformation);
		}
		else
		{
			OutStatus = Status == STATUS_INSUFFICIENT_RESOURCES
				? EOperationStringStatus::AllocationFailed
				: EOperationStringStatus::Unavailable;
		}
	}

	struct FCreateCompletionContext
	{
		Argus::Minifilter::FCreateOperationDescriptor Descriptor;
	};

	NTSTATUS FailureStatus(NTSTATUS RequestedStatus) noexcept
	{
		return NT_SUCCESS(RequestedStatus) ? STATUS_ACCESS_DENIED : RequestedStatus;
	}

	FLT_PREOP_CALLBACK_STATUS CompletePreWithStatus(PFLT_CALLBACK_DATA CallbackData, NTSTATUS Status) noexcept
	{
		CallbackData->IoStatus.Status = FailureStatus(Status);
		CallbackData->IoStatus.Information = 0;
		return FLT_PREOP_COMPLETE;
	}

	Argus::Minifilter::FCreateOperationDescriptor CreateDescriptor(PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects) noexcept
	{
		Argus::Minifilter::FCreateOperationDescriptor Descriptor{};
		const auto& Create = CallbackData->Iopb->Parameters.Create;
		const ULONG Options = Create.Options;

		Descriptor.RequestorPid = FltGetRequestorProcessId(CallbackData);
		KeQuerySystemTime(&Descriptor.ObservedAt);
		Descriptor.OperationFlags = CallbackData->Iopb->OperationFlags;
		Descriptor.SharedAccess = Create.ShareAccess;
		Descriptor.Attribute = Create.FileAttributes;
		Descriptor.Disposition = Options >> 24;
		Descriptor.DesiredAccess = Create.SecurityContext != nullptr
			? Create.SecurityContext->DesiredAccess : 0;

		const uint32 CreateOptions = Options & 0x00FFFFFFu;
		Descriptor.bSynchronous			 = (CreateOptions & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) != 0;
		Descriptor.bDirectoryRequired	 = (CreateOptions &  FILE_DIRECTORY_FILE		   ) != 0;
		Descriptor.bNonDirectoryRequired = (CreateOptions &  FILE_NON_DIRECTORY_FILE	   ) != 0;
		Descriptor.bNonCached			 = (CreateOptions &  FILE_NO_INTERMEDIATE_BUFFERING) != 0;
		Descriptor.bSequential			 = (CreateOptions &  FILE_SEQUENTIAL_ONLY		   ) != 0;
		Descriptor.bRandomAccess		 = (CreateOptions &  FILE_RANDOM_ACCESS			   ) != 0;
		Descriptor.bWriteThrough		 = (CreateOptions &  FILE_WRITE_THROUGH			   ) != 0;
		Descriptor.bDeleteOnClose		 = (CreateOptions &  FILE_DELETE_ON_CLOSE		   ) != 0;
		Descriptor.bOpenByFileId		 = (CreateOptions &  FILE_OPEN_BY_FILE_ID		   ) != 0;
		Descriptor.bOpenReparsePoint	 = (CreateOptions &  FILE_OPEN_REPARSE_POINT	   ) != 0;

		CaptureVolumeName(FltObjects != nullptr ? FltObjects->Volume : nullptr,
			Descriptor.VolumeName, Descriptor.VolumeNameStatus);
		CaptureFilePathName(CallbackData, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT,
			Descriptor.FilePathName, Descriptor.FilePathNameStatus);

		return Descriptor;
	}

	VOID FLTAPI OnPreOperationDeferredCallback(PFLT_DEFERRED_IO_WORKITEM WorkItem, PFLT_CALLBACK_DATA CallbackData, PVOID WorkerContext)
	{
		auto* Context = static_cast<Argus::Minifilter::FCreateOperationDescriptor*>(WorkerContext);
		if (Context == nullptr)
		{
			CompletePreWithStatus(CallbackData, STATUS_INVALID_PARAMETER);
			FltCompletePendedPreOperation(CallbackData, FLT_PREOP_COMPLETE, nullptr);
		}
		else
		{
			//auto  Decision = EvaluateDeferredPreDecision(*Context);
			auto  Decision = FCreateOperation::FRules::ApplyPreDecisionDeferred(GDriverContext->GetMinifilter(), *Context);
			if (Decision.Decision == Argus::Minifilter::ECreatePreOperationDecision::PreOperationPass)
			{
				ReleaseCreateContext(Context);
				FltCompletePendedPreOperation(CallbackData, FLT_PREOP_SUCCESS_NO_CALLBACK, nullptr);
			}
			else
			{
				const NTSTATUS Status = Decision.Decision == Argus::Minifilter::ECreatePreOperationDecision::PreOperationBlock
					? Decision.BlockStatus : STATUS_INVALID_PARAMETER;

				ReleaseCreateContext(Context);
				CompletePreWithStatus(CallbackData, Status);
				FltCompletePendedPreOperation(CallbackData, FLT_PREOP_COMPLETE, nullptr);
			}
		}

		FltFreeDeferredIoWorkItem(WorkItem);
	}

	VOID FLTAPI CompleteDeferredPost(PFLT_DEFERRED_IO_WORKITEM WorkItem, PFLT_CALLBACK_DATA CallbackData, PVOID WorkerContext)
	{
		auto* Context = static_cast<FCreateCompletionContext*>(WorkerContext);
		if (Context != nullptr)
		{
			DestructItem(Context);
			ExFreePoolWithTag(Context, CreateContextTag);
		}
		
		FltCompletePendedPostOperation(CallbackData);
		FltFreeDeferredIoWorkItem(WorkItem);
	}
}

FLT_PREOP_CALLBACK_STATUS FCreateOperation::OnPreOperationCallback(
	FMinifilter& Filter,
	PFLT_CALLBACK_DATA CallbackData,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID* CompletionContext) noexcept
{
	UNREFERENCED_PARAMETER(FltObjects);
	*CompletionContext = nullptr;

	const EFilterMode FilterMode = Filter.GetFilterMode();
	switch (FilterMode)
	{
	case EFilterMode::None:
	case EFilterMode::Pass:
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	case EFilterMode::Passive:
		return OnPreOperationFilterPassiveCallback(Filter, CallbackData, FltObjects, CompletionContext);
	case EFilterMode::Active:
		return OnPreOperationFilterActiveCallback(Filter, CallbackData, FltObjects, CompletionContext);
	default:
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
}

Argus::Minifilter::FCreatePreOperationDecision FCreateOperation::FRules::ApplyPreDecision(
	FMinifilter& Filter, const Argus::Minifilter::FCreateOperationDescriptor& Descriptor) noexcept
{
	UNREFERENCED_PARAMETER(Filter);
	UNREFERENCED_PARAMETER(Descriptor);

	Argus::Minifilter::FCreatePreOperationDecision Decision{};
	Decision.Decision = Argus::Minifilter::ECreatePreOperationDecision::PreOperationPass;
	return Decision;
}

Argus::Minifilter::FCreatePreOperationDecision FCreateOperation::FRules::ApplyPreDecisionDeferred(
	FMinifilter& Filter, const Argus::Minifilter::FCreateOperationDescriptor& Descriptor) noexcept
{
	UNREFERENCED_PARAMETER(Filter);
	UNREFERENCED_PARAMETER(Descriptor);

	Argus::Minifilter::FCreatePreOperationDecision Decision{};
	Decision.Decision = Argus::Minifilter::ECreatePreOperationDecision::PreOperationPass;
	return Decision;
}

FLT_POSTOP_CALLBACK_STATUS FCreateOperation::OnPostOperationCallback(
	FMinifilter& Filter,
	PFLT_CALLBACK_DATA CallbackData,
	PCFLT_RELATED_OBJECTS FltObjects,
	void* CompletionContext,
	FLT_POST_OPERATION_FLAGS Flags) noexcept
{
	auto* Context = static_cast<Argus::Minifilter::FCreateOperationDescriptor*>(CompletionContext);
	if (FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING))
	{
		ReleaseCreateContext(Context);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}
	if (Context != nullptr)
	{
		return OnPostOperationFilterPassiveCallback(Filter, CallbackData, FltObjects, CompletionContext, Flags);
	}
	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS FCreateOperation::OnPreOperationFilterPassiveCallback(
	FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
{
	UNREFERENCED_PARAMETER(Filter);

	auto* Context = static_cast<Argus::Minifilter::FCreateOperationDescriptor*>(
		ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(Argus::Minifilter::FCreateOperationDescriptor), CreateContextTag));
	if (Context == nullptr)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	auto  Descriptor = CreateDescriptor(CallbackData, FltObjects);
	ConstructItem(Context, MoveTemp(Descriptor));

	*CompletionContext = Context;
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS FCreateOperation::OnPreOperationFilterActiveCallback(
	FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
{
	UNREFERENCED_PARAMETER(FltObjects);

	auto Descriptor = CreateDescriptor(CallbackData, FltObjects);
	auto Decision = FRules::ApplyPreDecision(Filter, Descriptor);
	Descriptor.PreOperationDecision = Decision;

	switch (Decision.Decision)
	{
	case Argus::Minifilter::ECreatePreOperationDecision::PreOperationPass:
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	case Argus::Minifilter::ECreatePreOperationDecision::PreOperationBlock:
		return CompletePreWithStatus(CallbackData, Decision.BlockStatus);
	case Argus::Minifilter::ECreatePreOperationDecision::PreOperationPending:
	{
		const bool bCanPendIrp = FLT_IS_IRP_OPERATION(CallbackData);
		if (!bCanPendIrp)
		{
			return CompletePreWithStatus(CallbackData, STATUS_INVALID_DEVICE_REQUEST);
		}

		auto* Context = static_cast<Argus::Minifilter::FCreateOperationDescriptor*>(
			ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(Argus::Minifilter::FCreateOperationDescriptor), CreateContextTag));
		if (Context == nullptr)
		{
			return CompletePreWithStatus(CallbackData, STATUS_INSUFFICIENT_RESOURCES);
		}

		ConstructItem(Context, MoveTemp(Descriptor));
		*CompletionContext = Context;

		auto* WorkItem = FltAllocateDeferredIoWorkItem();
		if (WorkItem == nullptr)
		{
			ReleaseCreateContext(Context);
			return CompletePreWithStatus(CallbackData, STATUS_INSUFFICIENT_RESOURCES);
		}
		const NTSTATUS Status = FltQueueDeferredIoWorkItem(
			WorkItem, CallbackData, OnPreOperationDeferredCallback, DelayedWorkQueue, Context);
		if (!NT_SUCCESS(Status))
		{
			FltFreeDeferredIoWorkItem(WorkItem);
			ReleaseCreateContext(Context);
			return CompletePreWithStatus(CallbackData, Status);
		}

		return FLT_PREOP_PENDING;
	}
	default:
		// Warning
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
}

FLT_POSTOP_CALLBACK_STATUS FCreateOperation::OnPostOperationFilterPassiveCallback(
	FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, void* CompletionContext, FLT_POST_OPERATION_FLAGS Flags) noexcept
{
	UNREFERENCED_PARAMETER(Flags);

	auto* Context = static_cast<Argus::Minifilter::FCreateOperationDescriptor*>(CompletionContext);
	if (Context == nullptr)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	Argus::Minifilter::FCreateOperationDescriptor& Descriptor = *Context;
	Descriptor.CompletionInformation = CallbackData->IoStatus.Information;
	Descriptor.CompletionStatus = CallbackData->IoStatus.Status;
	if (NT_SUCCESS(Descriptor.CompletionStatus) && Descriptor.CompletionStatus != STATUS_REPARSE)
	{
		CaptureVolumeName(FltObjects != nullptr ? FltObjects->Volume : nullptr,
			Descriptor.CompletionVolumeName, Descriptor.CompletionVolumeNameStatus);
		CaptureFilePathName(CallbackData, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
			Descriptor.CompletionFilePathName, Descriptor.CompletionFilePathNameStatus);
	}

	FCreateOperationContainer& Container = Filter.GetOperationContainer().Get<Argus::Minifilter::EOperation::Create>();
	Container.Push(MoveTemp(Descriptor));
	ReleaseCreateContext(Context);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

Argus::Minifilter::FCreatePostOperationDecision FCreateOperation::FRules::ApplyPostDecision(
	PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, const Argus::Minifilter::FCreateOperationDescriptor& Descriptor) noexcept
{	
	UNREFERENCED_PARAMETER(CallbackData);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Descriptor);

	Argus::Minifilter::FCreatePostOperationDecision Decision{};
	Decision.Decision = Argus::Minifilter::ECreatePostOperationDecision::PostOperationPass;
	return Decision;
}
