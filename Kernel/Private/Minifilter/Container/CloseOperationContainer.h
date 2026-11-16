#pragma once

#include "Kernel/Private/Minifilter/Container/OperationContainerBase.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Public/Containers/KrnlString.h"
#include "Kernel/Public/Misc/KernelTypes.h"

namespace Argus::Minifilter
{
	struct FCloseOperationDescriptor
	{
		uint64 OperationId = 0;

		uint32 RequestorPid = 0;
		uint32 RequestorTid = 0;
		EProcessorMode ProcessorMode = EProcessorMode::Unknown;

		LARGE_INTEGER ObservedAt{};
	};
}

class FCloseOperationContainer :
	public FOperationContainer<Argus::Minifilter::FCloseOperationDescriptor, EPoolType::Paged, Argus::Minifilter::ELockType::Wait>
{
public:
	using Super = FOperationContainer<Argus::Minifilter::FCloseOperationDescriptor, EPoolType::Paged, Argus::Minifilter::ELockType::Wait>;
};
