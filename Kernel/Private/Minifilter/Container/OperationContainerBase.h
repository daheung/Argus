#pragma once

#include "Core/Public/Templates/TypeTraits.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"

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
	FLockType Lock;
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

template <typename OperationDescriptor, Argus::Minifilter::ELockType InLockType>
class IOperationContainerBase
{
public:
	static constexpr Argus::Minifilter::ELockType LockType = InLockType;

	typedef OperationDescriptor FDescriptorType;
	typedef TOperationContainerLock<LockType> FLockType;

protected:
	IOperationContainerBase() = default;

	typedef typename FLockType::FReadGuard  FReadGuard;
	typedef typename FLockType::FWriteGuard FWriteGuard;

	FLockType Lock;
};
