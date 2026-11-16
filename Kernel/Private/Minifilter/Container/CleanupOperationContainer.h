#pragma once

#include "Kernel/Private/Minifilter/Container/OperationContainerBase.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Public/Containers/KrnlString.h"
#include "Kernel/Public/Misc/KernelTypes.h"

namespace Argus::Minifilter
{
	struct FCleanupOperationDescriptor
	{
		uint64 OperationId = 0;

		uint32 RequestorPid = 0;
		uint32 RequestorTid = 0;
		EProcessorMode ProcessorMode = EProcessorMode::Unknown;

		LARGE_INTEGER ObservedAt{};
		LARGE_INTEGER CompletionObservedAt{};

		bool bIsCompletionCleanup;
		NTSTATUS CompletionStatus;
		uint64 CompletionInformation;
	};
}

class FCleanupOperationContainer :
	public FOperationContainer<Argus::Minifilter::FCleanupOperationDescriptor, EPoolType::NonPaged, Argus::Minifilter::ELockType::Spinlock>
{
public:
	using Super = FOperationContainer<Argus::Minifilter::FCleanupOperationDescriptor, EPoolType::NonPaged, Argus::Minifilter::ELockType::Spinlock>;
};
