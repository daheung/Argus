#pragma once
#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Private/Minifilter/Container/CleanupOperationContainer.h"

class FMinifilter;

class FCleanupOperation
{
public:
	static FLT_PREOP_CALLBACK_STATUS OnPreOperationCallback(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext) noexcept;

	static FLT_POSTOP_CALLBACK_STATUS OnPostOperationCallback(FMinifilter& Filter, PFLT_CALLBACK_DATA CallbackData, PCFLT_RELATED_OBJECTS FltObjects, void* CompletionContext, FLT_POST_OPERATION_FLAGS Flags) noexcept;
};