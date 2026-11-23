#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"

namespace Argus::Minifilter
{
	struct FMinifilterContext
	{
		uint64 ContextId;
		uint32 ContextProcessId;
		int64  ContextProcessCreatedAt;
		int64  ContextCreatedAt;
	};

	enum class EPreOperationDecision : uint8
	{
		PreOperationPass,
		PreOperationBlock,
		PreOperationPending,
	};

	enum class EPostOperationDecision : uint8
	{
		PostOperationNone,
		PostOperationPass,
		PostOperationBlock,
		PostOperationMoreProcessingRequired,
	};

	struct FPreOperationDecision
	{
		EPreOperationDecision Decision = EPreOperationDecision::PreOperationPass;
		NTSTATUS BlockStatus = STATUS_ACCESS_DENIED;
	};

	struct FPostOperationDecision
	{
		EPostOperationDecision Decision = EPostOperationDecision::PostOperationPass;
		NTSTATUS BlockStatus = STATUS_ACCESS_DENIED;
	};
	
	struct FOperationIoStatus
	{
		NTSTATUS Status;
		uint64 Information;
	};
}

