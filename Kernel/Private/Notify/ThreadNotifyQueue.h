#pragma once

#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Private/Notify/ThreadNotifyInfo.h"

class FThreadNotifyQueue
{
public:
	explicit FThreadNotifyQueue(uint32 Tag) noexcept
		: ThreadNotifyInfos(Tag)
		, bInitSucceed(0)
		, TryRestoreCount(0)
	{
		ExInitializePushLock(&this->ThreadNotifyInfosLock);
		if (this->ThreadNotifyInfos.Initialize(ThreadNotifyMaxInfos) == EContainerStatus::Success)
		{
			this->bInitSucceed.store(1, Atomic::EMemoryOrder::Release);
		}
	}

	FThreadNotifyQueue(const FThreadNotifyQueue&) = delete;
	FThreadNotifyQueue& operator=(const FThreadNotifyQueue&) = delete;

	void OnThreadCreateNotify(HANDLE ProcessId, HANDLE ThreadId);
	void OnThreadExitNotify(HANDLE ProcessId, HANDLE ThreadId);

	bool TryPop(Argus::Notify::FThreadNotifyInfo& OutInfo) noexcept
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&this->ThreadNotifyInfosLock);

		const bool bSuccess = this->ThreadNotifyInfos.TryPop(OutInfo);

		ExReleasePushLockExclusive(&this->ThreadNotifyInfosLock);
		KeLeaveCriticalRegion();

		return bSuccess;
	}

	uint64 GetDroppedCount() noexcept
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockShared(&this->ThreadNotifyInfosLock);
		
		const uint64 Count = this->DroppedThreadNotifyCount;

		ExReleasePushLockShared(&this->ThreadNotifyInfosLock);
		KeLeaveCriticalRegion();
		return Count;
	}

private:
	void OnThreadNotifyImpl(HANDLE ProcessId, HANDLE ThreadId, bool bCreate) noexcept;
	bool TryRestore() noexcept
	{
		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Acquire) != 0)
		{
			return true;
		}

		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&this->ThreadNotifyInfosLock);

		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Relaxed) == 0 &&
			this->TryRestoreCount < TryMaxRestoreCount)
		{
			if (this->ThreadNotifyInfos.Initialize(ThreadNotifyMaxInfos) == EContainerStatus::Success)
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
			++this->DroppedThreadNotifyCount;
		}

		ExReleasePushLockExclusive(&this->ThreadNotifyInfosLock);
		KeLeaveCriticalRegion();
		return bReady;
	}

public:
	static constexpr uint16 ThreadNotifyMaxInfos = 2048;
	static constexpr uint16 TryMaxRestoreCount = 3;

private:
	TSpscCircularQueue<EPoolType::Paged, Argus::Notify::FThreadNotifyInfo> ThreadNotifyInfos;
	EX_PUSH_LOCK ThreadNotifyInfosLock;
	uint64 DroppedThreadNotifyCount = 0;

	TKernelAtomicCounter<uint32> bInitSucceed;
	uint32 TryRestoreCount;
};
