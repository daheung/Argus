#pragma once

#include "Core/Public/Templates/TypeTraits.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlContainerLock.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"

template <typename OperationDescriptor, EPoolType InPoolType, EContainerLockType InLockType>
class FOperationContainer
{
	static_assert(InLockType != EContainerLockType::Spinlock || InPoolType == EPoolType::NonPaged,
		"Spinlock-protected operation containers require nonpaged queue storage");

public:
	static constexpr EPoolType PoolType = InPoolType;
	static constexpr EContainerLockType LockType = InLockType;
	static constexpr int32 MaxEvents = 1024;

	typedef OperationDescriptor DescriptorType;
	typedef DescriptorType FDescriptorType;
	typedef TKrnlContainerLock<LockType> FLockType;
	typedef TSpscCircularQueue<PoolType, DescriptorType> TQueue;

public:
	FOperationContainer() = default;

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

	template <typename PredicateType>
	NODISCARD bool TryPopByPredicate(PredicateType&& Predicate, DescriptorType& OutDescriptor) noexcept
	{
		FWriteGuard Guard(this->Lock);
		return bInitialized && Container.TryPopByPredicate(Forward<PredicateType>(Predicate), OutDescriptor);
	}

	template <typename VisitorType>
	FORCEINLINE uint32 ForEach(VisitorType&& Visitor) const noexcept
	{
		FReadGuard Guard(this->Lock);
		if (!bInitialized)
		{
			return static_cast<uint32>(INDEX_NONE);
		}

		using USizeType = typename TMakeUnsigned<typename TQueue::SizeType>::Type;
		USizeType Index = 0;
		for (; Index < static_cast<USizeType>(Container.Num()); ++Index)
		{
			const auto& Descriptor = Container[Index];
			if (!(::Invoke(Visitor, Descriptor, Index)))
			{
				break;
			}
		}

		return static_cast<uint32>(Index);
	}

	NODISCARD bool IsEmpty() const noexcept
	{
		FReadGuard Guard(this->Lock);
		return Container.IsEmpty();
	}

	NODISCARD uint64 GetDroppedCount() const noexcept
	{
		FReadGuard Guard(this->Lock);
		return DroppedCount;
	}

private:
	typedef typename FLockType::FReadGuard  FReadGuard;
	typedef typename FLockType::FWriteGuard FWriteGuard;

	mutable FLockType Lock;
	TQueue Container;
	uint64 DroppedCount = 0;
	bool bInitialized = false;
};
