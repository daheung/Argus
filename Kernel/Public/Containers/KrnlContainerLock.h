#pragma once

#include "Core/Public/Misc/Assertion.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerType.h"

// Wait guards require IRQL <= APC_LEVEL. Spin guards raise to DISPATCH_LEVEL;
// the spin lock and all data accessed while held must reside in nonpaged memory.
template <EContainerLockType ContainerLockType>
class TKrnlContainerLock;

template <>
class TKrnlContainerLock<EContainerLockType::Wait>
{
public:
	static constexpr EContainerLockType ContainerLockType = EContainerLockType::Wait;
	typedef EX_PUSH_LOCK FLockType;

public:
	TKrnlContainerLock()
	{
		ExInitializePushLock(&this->Lock);
	}

	TKrnlContainerLock(const TKrnlContainerLock&) = delete;
	TKrnlContainerLock& operator=(const TKrnlContainerLock&) = delete;

public:
	template <bool bShared>
	class TGuard
	{
	public:
		FORCEINLINE explicit TGuard(TKrnlContainerLock& InOwner)
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
		TKrnlContainerLock& Owner;
	};

	typedef TGuard<true > FReadGuard;
	typedef TGuard<false> FWriteGuard;

private:
	mutable FLockType Lock;
};

template <>
class TKrnlContainerLock<EContainerLockType::Spinlock>
{
public:
	static constexpr EContainerLockType ContainerLockType = EContainerLockType::Spinlock;
	typedef KSPIN_LOCK FLockType;

public:
	TKrnlContainerLock() noexcept
	{
		KeInitializeSpinLock(&Lock);
	}

	TKrnlContainerLock(const TKrnlContainerLock&) = delete;
	TKrnlContainerLock& operator=(const TKrnlContainerLock&) = delete;

	class FGuard
	{
	public:
		explicit FGuard(TKrnlContainerLock& InOwner) noexcept
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
		TKrnlContainerLock& Owner;
		KIRQL PreviousIrql{};
	};

	// A spin lock has no shared mode; reads also acquire it exclusively.
	using FReadGuard  = FGuard;
	using FWriteGuard = FGuard;

private:
	FLockType Lock;
};
