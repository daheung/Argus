#pragma once

#include "Core/Public/Templates/TypeTraits.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"

namespace Argus::Minifilter
{
	enum class ELockType : uint8
	{
		Wait,
		Spinlock
	};
};

template <Argus::Minifilter::ELockType>
class TOperationContainerLock;

template <>
class TOperationContainerLock<Argus::Minifilter::ELockType::Wait>
{
public:
	typedef EX_PUSH_LOCK FLockType;

public:
	TOperationContainerLock()
	{
		ExInitializePushLock(&this->Lock);
	}

	TOperationContainerLock(const TOperationContainerLock&) = delete;
	TOperationContainerLock& operator=(const TOperationContainerLock&) = delete;

public:
	template <bool bShared>
	class TGuard
	{
	public:
		FORCEINLINE explicit TGuard(TOperationContainerLock& InOwner)
			: Owner(InOwner)
		{
			CORE_CHECK(KeGetCurrentIrql() <= APC_LEVEL);
			KeEnterCriticalRegion();

			if constexpr (bShared)
			{
				ExAcquirePushLockShared(&Owner.Lock);
			}
			else
			{
				ExAcquirePushLockExclusive(&Owner.Lock);
			}
		}

		FORCEINLINE ~TGuard() noexcept
		{
			if constexpr (bShared)
			{
				ExReleasePushLockShared(&Owner.Lock);
			}
			else
			{
				ExReleasePushLockExclusive(&Owner.Lock);
			}

			KeLeaveCriticalRegion();
		}

		TGuard(const TGuard&) = delete;
		TGuard& operator=(const TGuard&) = delete;
		TGuard(TGuard&&) = delete;
		TGuard& operator=(TGuard&&) = delete;

	private:
		TOperationContainerLock& Owner;
	};

	typedef TGuard<true > FReadGuard;
	typedef TGuard<false> FWriteGuard;

private:
	mutable FLockType Lock;
};

template <>
class TOperationContainerLock<Argus::Minifilter::ELockType::Spinlock>
{
public:
	TOperationContainerLock() noexcept
	{
		KeInitializeSpinLock(&Lock);
	}

	TOperationContainerLock(const TOperationContainerLock&) = delete;
	TOperationContainerLock& operator=(const TOperationContainerLock&) = delete;

	class FGuard
	{
	public:
		explicit FGuard(TOperationContainerLock& InOwner) noexcept
			: Owner(InOwner)
		{
			CORE_CHECK(KeGetCurrentIrql() <= DISPATCH_LEVEL);
			KeAcquireSpinLock(&Owner.Lock, &PreviousIrql);
		}

		~FGuard() noexcept
		{
			KeReleaseSpinLock(&Owner.Lock, PreviousIrql);
		}

		FGuard(const FGuard&) = delete;
		FGuard& operator=(const FGuard&) = delete;
		FGuard(FGuard&&) = delete;
		FGuard& operator=(FGuard&&) = delete;

	private:
		TOperationContainerLock& Owner;
		KIRQL PreviousIrql{};
	};

	// A spin lock has no shared mode; reads also acquire it exclusively.
	using FReadGuard = FGuard;
	using FWriteGuard = FGuard;

private:
	KSPIN_LOCK Lock;
};

template <typename OperationDescriptor, EPoolType InPoolType, Argus::Minifilter::ELockType InLockType>
class FOperationContainer
{
	static_assert(InLockType != Argus::Minifilter::ELockType::Spinlock || InPoolType == EPoolType::NonPaged,
		"Spinlock-protected operation containers require nonpaged queue storage");

public:
	static constexpr EPoolType PoolType = InPoolType;
	static constexpr Argus::Minifilter::ELockType LockType = InLockType;
	static constexpr int32 MaxEvents = 1024;

	typedef OperationDescriptor DescriptorType;
	typedef DescriptorType FDescriptorType;
	typedef TOperationContainerLock<LockType> FLockType;
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
