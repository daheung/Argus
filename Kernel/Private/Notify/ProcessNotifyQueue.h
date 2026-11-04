#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Private/Notify/ProcessNotifyInfo.h"

class FProcessNotifyQueue
{
public:
	explicit FProcessNotifyQueue(uint32 Tag) noexcept
		: ProcessNotifyInfos(Tag)
		, bInitSucceed(0)
		, TryRestoreCount(0)
	{
		ExInitializePushLock(&this->ProcessNotifyInfosLock);
		if (this->ProcessNotifyInfos.Initialize(ProcessNotifyMaxInfos) == EContainerStatus::Success)
		{
			this->bInitSucceed.store(1, Atomic::EMemoryOrder::Release);
		}
	}

	FProcessNotifyQueue(const FProcessNotifyQueue&) = delete;
	FProcessNotifyQueue& operator=(const FProcessNotifyQueue&) = delete;

	void OnProcessCreateNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
	void OnProcessExitNotify(PEPROCESS Process, HANDLE ProcessId);

	bool TryPop(Argus::Notify::FProcessNotifyInfo& OutInfo) noexcept
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&this->ProcessNotifyInfosLock);

		const bool bSuccess = this->ProcessNotifyInfos.TryPop(OutInfo);

		ExReleasePushLockExclusive(&this->ProcessNotifyInfosLock);
		KeLeaveCriticalRegion();

		return bSuccess;
	}

	uint64 GetDroppedCount() noexcept
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockShared(&this->ProcessNotifyInfosLock);
		const uint64 Count = this->DroppedProcessNotifyCount;
		ExReleasePushLockShared(&this->ProcessNotifyInfosLock);
		KeLeaveCriticalRegion();
		return Count;
	}

public:
	//static constexpr uint16 ProcessNotifyMaxCommandLine = Argus::Notify::ProcessNotifyMaxCommandLineBytes;
	static constexpr uint16 ProcessNotifyMaxInfos = 1024;
	static constexpr uint16 TryMaxRestoreCount = 3;

private:
	bool TryRestore() noexcept
	{
		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Acquire) != 0)
		{
			return true;
		}

		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&this->ProcessNotifyInfosLock);

		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Relaxed) == 0 &&
			this->TryRestoreCount < TryMaxRestoreCount)
		{
			if (this->ProcessNotifyInfos.Initialize(ProcessNotifyMaxInfos) == EContainerStatus::Success)
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
			++this->DroppedProcessNotifyCount;
		}

		ExReleasePushLockExclusive(&this->ProcessNotifyInfosLock);
		KeLeaveCriticalRegion();
		return bReady;
	}

private:
	TSpscCircularQueue<EPoolType::Paged, Argus::Notify::FProcessNotifyInfo> ProcessNotifyInfos;
	EX_PUSH_LOCK ProcessNotifyInfosLock;
	uint64 DroppedProcessNotifyCount = 0;

	TKernelAtomicCounter<uint32> bInitSucceed;
	uint32 TryRestoreCount;
};
