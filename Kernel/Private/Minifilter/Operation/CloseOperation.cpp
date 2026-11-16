#include "Kernel/Private/Minifilter/Operation/CloseOperation.h"
#include "Kernel/Private/Minifilter/MinifilterTypes.h"
#include "Kernel/Private/Minifilter/Minifilter.h"
#include "Kernel/Public/Misc/KernelMisc.h"

namespace
{
	void SnapshotPreOperation(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
	{
		UNREFERENCED_PARAMETER(CompletionContext);

		Argus::Minifilter::FMinifilterContext Context{};
		const NTSTATUS Status = Filter.ReadStreamHandleContext(FltObjects, Context);
		if (!NT_SUCCESS(Status))
		{
			return;
		}

		uint32 RequestorTid = 0;
		if (CallbackData->Thread != nullptr)
		{
			RequestorTid = HandleToULong(PsGetThreadId(CallbackData->Thread));
		}

		Argus::Minifilter::FCloseOperationDescriptor Descriptor;
		KeQuerySystemTimePrecise(&Descriptor.ObservedAt);
		Descriptor.OperationId = Context.ContextId;
		Descriptor.ProcessorMode = ProcessorModeFrom(CallbackData->RequestorMode);
		Descriptor.RequestorPid = FltGetRequestorProcessId(CallbackData);
		Descriptor.RequestorTid = RequestorTid;

		auto& Container = Filter.GetOperationContainer().Get<Argus::Minifilter::EOperation::Close>();
		Container.Push(MoveTemp(Descriptor));
	}
}

FLT_PREOP_CALLBACK_STATUS FCloseOperation::OnPreOperationCallback(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
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

FLT_PREOP_CALLBACK_STATUS FCloseOperation::OnPreOperationFilterPassiveCallback(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
{
	SnapshotPreOperation(Filter, CallbackData, FltObjects, CompletionContext);
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS FCloseOperation::OnPreOperationFilterActiveCallback(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept
{
	SnapshotPreOperation(Filter, CallbackData, FltObjects, CompletionContext);
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
