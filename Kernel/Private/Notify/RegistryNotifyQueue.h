#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Private/Notify/RegistryNotifyInfo.h"
#include "Kernel/Private/Callback/RegistryOperationContext.h"
#include "Kernel/Private/Notify/LockType.h"

class FRegistryNotifyQueue
{
public:
	typedef TSpscCircularQueue<EPoolType::Paged, Argus::Notify::FRegistryNotifyInfo> TQueue;

public:
	explicit FRegistryNotifyQueue(uint32 Tag) noexcept
		: RegistryNotifyInfos(Tag)
		, NextOperationId(1)
		, bInitSucceed(0)
		, TryRestoreCount(0)
	{
		ExInitializePushLock(&this->RegistryNotifyInfosLock);
		if (this->RegistryNotifyInfos.Initialize(RegistryNotifyMaxInfos) == EContainerStatus::Success)
		{
			this->bInitSucceed.store(1, Atomic::EMemoryOrder::Release);
		}
	}

	FRegistryNotifyQueue(const FRegistryNotifyQueue&) = delete;
	FRegistryNotifyQueue& operator=(const FRegistryNotifyQueue&) = delete;

	void OnRegistryNotify(NTSTATUS CallbackStatus, void* Context,
		void* ArgumentA, void* ArgumentB) noexcept;

	FORCEINLINE void Lock(ELockType LockType) noexcept
	{
		KeEnterCriticalRegion();

		if (LockType == ELockType::Shared)
		{
			ExAcquirePushLockShared(&this->RegistryNotifyInfosLock);
		}
		else
		{
			ExAcquirePushLockExclusive(&this->RegistryNotifyInfosLock);
		}
	}

	FORCEINLINE void Unlock(ELockType LockType) noexcept
	{
		if (LockType == ELockType::Shared)
		{
			ExReleasePushLockShared(&this->RegistryNotifyInfosLock);
		}
		else
		{
			ExReleasePushLockExclusive(&this->RegistryNotifyInfosLock);
		}

		KeLeaveCriticalRegion();
	}

public:
	FORCEINLINE uint32 QueueNum() const noexcept
	{
		return static_cast<uint32>(this->RegistryNotifyInfos.Num());
	}

	FORCEINLINE Argus::Notify::FRegistryNotifyInfo& TryPeek() noexcept
	{
		return this->RegistryNotifyInfos.PeekFront();
	}

	FORCEINLINE bool TryPop(Argus::Notify::FRegistryNotifyInfo& OutInfo) noexcept
	{
		return this->RegistryNotifyInfos.TryPop(OutInfo);
	}

	NODISCARD FORCEINLINE TQueue& GetQueue() noexcept
	{
		return this->RegistryNotifyInfos;
	}

	uint64 GetDroppedCount() noexcept
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockShared(&this->RegistryNotifyInfosLock);
		const uint64 Count = this->DroppedRegistryNotifyCount;
		ExReleasePushLockShared(&this->RegistryNotifyInfosLock);
		KeLeaveCriticalRegion();
		return Count;
	}

public:
	static constexpr uint16 RegistryNotifyMaxInfos = 1024;
	static constexpr uint16 TryMaxRestoreCount = 3;

private:
	void OnPreRegistryNotify(const FRegistryOperationContext& Operation, NTSTATUS DecisionStatus) noexcept;
	void OnPostRegistryNotify(const FRegistryOperationContext& Operation) noexcept;
	void EnqueueReady(Argus::Notify::FRegistryNotifyInfo&& Info) noexcept;

	bool TryRestore() noexcept
	{
		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Acquire) != 0)
		{
			return true;
		}

		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&this->RegistryNotifyInfosLock);

		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Relaxed) == 0 &&
			this->TryRestoreCount < TryMaxRestoreCount)
		{
			if (this->RegistryNotifyInfos.Initialize(RegistryNotifyMaxInfos) == EContainerStatus::Success)
			{
				this->bInitSucceed.store(1, Atomic::EMemoryOrder::Release);
			}
			else
			{
				++this->TryRestoreCount;
			}
		}

		const bool bReady = this->bInitSucceed.load(Atomic::EMemoryOrder::Relaxed) != 0;
		if (!bReady)
		{
			++this->DroppedRegistryNotifyCount;
		}

		ExReleasePushLockExclusive(&this->RegistryNotifyInfosLock);
		KeLeaveCriticalRegion();
		return bReady;
	}

private:
	TSpscCircularQueue<EPoolType::Paged, Argus::Notify::FRegistryNotifyInfo> RegistryNotifyInfos;
	EX_PUSH_LOCK RegistryNotifyInfosLock;
	uint64 DroppedRegistryNotifyCount = 0;

	TKernelAtomicCounter<uint64> NextOperationId;
	TKernelAtomicCounter<uint32> bInitSucceed;
	uint32 TryRestoreCount;
};
