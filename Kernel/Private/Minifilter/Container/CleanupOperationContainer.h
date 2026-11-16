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
	};
}

class FCleanupOperationContainer :
	public IOperationContainerBase<Argus::Minifilter::FCleanupOperationDescriptor, Argus::Minifilter::ELockType::Spinlock>
{
public:
	typedef IOperationContainerBase<Argus::Minifilter::FCleanupOperationDescriptor, Argus::Minifilter::ELockType::Spinlock> Super;

	typedef Argus::Minifilter::FCleanupOperationDescriptor DescriptorType;
	typedef TOperationContainerLock<Argus::Minifilter::ELockType::Spinlock> LockType;
	typedef TSpscCircularQueue<EPoolType::Paged, DescriptorType> TQueue;

public:
	FCleanupOperationContainer()
		: Super()
		, Container()
	{
	}

public:
	NODISCARD bool Initialize() noexcept
	{
		FWriteGuard Guard(this->Lock);
		if (bInitialized)
		{
			return true;
		}

		bInitialized = Container.Initialize(MaxEvents) == EContainerStatus::Success;
		return bInitialized;
	}

	FORCEINLINE bool Push(DescriptorType&& Descriptor)
	{
		FWriteGuard Guard(this->Lock);
		if (!bInitialized || Container.Push(MoveTempIfPossible(Descriptor)) == INDEX_NONE)
		{
			++DroppedCount;
			return false;
		}
		return true;
	}

	NODISCARD bool TryPop(DescriptorType& OutDescriptor) noexcept
	{
		FWriteGuard Guard(this->Lock);
		return bInitialized && Container.TryPopConsumer([&OutDescriptor](DescriptorType& QueuedDescriptor) noexcept
			{
				DestructItem(&OutDescriptor);
				ConstructItem(&OutDescriptor, MoveTemp(QueuedDescriptor));
			});
	}

	template <typename Predicate>
	NODISCARD bool TryPopByPredicate(Predicate&& Pred, DescriptorType& OutDescriptor) noexcept
	{
		FWriteGuard Guard(this->Lock);
		return bInitialized && Container.TryPopByPredicate(Forward<Predicate>(Pred), OutDescriptor);
	}

	NODISCARD FORCEINLINE bool IsEmpty() noexcept
	{
		FReadGuard Guard(this->Lock);
		return Container.IsEmpty();
	}

	NODISCARD uint64 GetDroppedCount() noexcept
	{
		FReadGuard Guard(this->Lock);
		return DroppedCount;
	}

	static constexpr int32 MaxEvents = 1024;

private:
	TQueue Container;
	uint64 DroppedCount = 0;
	bool bInitialized = false;
};
