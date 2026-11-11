#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Private/Minifilter/Container/CreateOperationContainer.h"

class FMinifilter;

class FCreateOperation
{
public:
	static FLT_PREOP_CALLBACK_STATUS OnPreOperationCallback(
		FMinifilter& Filter,
		PFLT_CALLBACK_DATA CallbackData,
		PCFLT_RELATED_OBJECTS FltObjects,
		PVOID* CompletionContext) noexcept;

	static FLT_POSTOP_CALLBACK_STATUS OnPostOperationCallback(
		FMinifilter& Filter,
		PFLT_CALLBACK_DATA CallbackData,
		PCFLT_RELATED_OBJECTS FltObjects,
		void* CompletionContext,
		FLT_POST_OPERATION_FLAGS Flags) noexcept;

private:
	static FLT_PREOP_CALLBACK_STATUS OnPreOperationFilterPassiveCallback(
		FMinifilter& Filter,
		PFLT_CALLBACK_DATA CallbackData,
		PCFLT_RELATED_OBJECTS FltObjects,
		PVOID* CompletionContext) noexcept;

	static FLT_PREOP_CALLBACK_STATUS OnPreOperationFilterActiveCallback(
		FMinifilter& Filter,
		PFLT_CALLBACK_DATA CallbackData,
		PCFLT_RELATED_OBJECTS FltObjects,
		PVOID* CompletionContext) noexcept;

	static FLT_POSTOP_CALLBACK_STATUS OnPostOperationFilterPassiveCallback(
		FMinifilter& Filter,
		PFLT_CALLBACK_DATA CallbackData,
		PCFLT_RELATED_OBJECTS FltObjects,
		void* CompletionContext,
		FLT_POST_OPERATION_FLAGS Flags) noexcept;

public:
	class FRules
	{
	public:
		// The policy evaluator can pass its decision to this dispatcher.
		static Argus::Minifilter::FCreatePreOperationDecision ApplyPreDecision(
			FMinifilter& Filter, const Argus::Minifilter::FCreateOperationDescriptor& Descriptor) noexcept;

		static Argus::Minifilter::FCreatePreOperationDecision ApplyPreDecisionDeferred(
			FMinifilter& Filter, const Argus::Minifilter::FCreateOperationDescriptor& Descriptor) noexcept;

		static Argus::Minifilter::FCreatePostOperationDecision ApplyPostDecision(
			PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, const Argus::Minifilter::FCreateOperationDescriptor& Descriptor) noexcept;
	};
};
