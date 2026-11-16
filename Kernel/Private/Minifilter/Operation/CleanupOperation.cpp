#include "Kernel/Private/Minifilter/Operation/CleanupOperation.h"
#include "Kernel/Private/Minifilter/Minifilter.h"
#include "Kernel/Private/Minifilter/MinifilterTypes.h"
#include "Kernel/Public/Misc/KernelMisc.h"

namespace
{
	constexpr uint32 CleanupContextTag = 'rCFk';

	FLT_PREOP_CALLBACK_STATUS OnPreOperationCallbackImpl(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
	{ 
		UNREFERENCED_PARAMETER(CompletionContext);

		Argus::Minifilter::FMinifilterContext HandleStreamContext{};
		const NTSTATUS Status = Filter.ReadStreamHandleContext(FltObjects, HandleStreamContext);
		if (!NT_SUCCESS(Status))
		{
			return FLT_PREOP_SUCCESS_NO_CALLBACK;
		}

		uint32 RequestorTid = 0;
		if (CallbackData->Thread != nullptr)
		{
			RequestorTid = HandleToULong(PsGetThreadId(CallbackData->Thread));
		}

		auto* Context = static_cast<Argus::Minifilter::FCleanupOperationDescriptor*>(
			ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(Argus::Minifilter::FCleanupOperationDescriptor), CleanupContextTag));
		if (Context == nullptr)
		{
			return FLT_PREOP_SUCCESS_NO_CALLBACK;
		}

		Argus::Minifilter::FCleanupOperationDescriptor Descriptor{};
		KeQuerySystemTimePrecise(&Descriptor.ObservedAt);
		Descriptor.OperationId   = HandleStreamContext.ContextId;
		Descriptor.ProcessorMode = ProcessorModeFrom(CallbackData->RequestorMode);
		Descriptor.RequestorPid  = FltGetRequestorProcessId(CallbackData);
		Descriptor.RequestorTid  = RequestorTid;

		ConstructItem(Context, MoveTemp(Descriptor));
		*CompletionContext = Context;

		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

	FLT_POSTOP_CALLBACK_STATUS OnPostOperationCallbackImpl(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, void* CompletionContext, FLT_POST_OPERATION_FLAGS Flags) noexcept
	{
		UNREFERENCED_PARAMETER(FltObjects);

		auto* Context = static_cast<Argus::Minifilter::FCleanupOperationDescriptor*>(CompletionContext);
		if (!FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING))
		{
			if (Context != nullptr)
			{
				KeQuerySystemTimePrecise(&Context->CompletionObservedAt);
				Context->bIsCompletionCleanup  = true;
				Context->CompletionInformation = CallbackData->IoStatus.Information;
				Context->CompletionStatus	   = CallbackData->IoStatus.Status;

				auto& Container = Filter.GetOperationContainer().Get<Argus::Minifilter::EOperation::Cleanup>();
				Container.Push(MoveTemp(*Context));
			}
		}

		if (Context != nullptr)
		{
			DestructItem(Context);
			ExFreePoolWithTag(Context, CleanupContextTag);
		}

		return FLT_POSTOP_FINISHED_PROCESSING;
	}
}

FLT_PREOP_CALLBACK_STATUS FCleanupOperation::OnPreOperationCallback(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
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
	case EFilterMode::Active:
		return OnPreOperationCallbackImpl(Filter, CallbackData, FltObjects, CompletionContext);
	default:
		// ??
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
}

FLT_POSTOP_CALLBACK_STATUS FCleanupOperation::OnPostOperationCallback(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, void* CompletionContext, FLT_POST_OPERATION_FLAGS Flags) noexcept
{
	return OnPostOperationCallbackImpl(Filter, CallbackData, FltObjects, CompletionContext, Flags);
}