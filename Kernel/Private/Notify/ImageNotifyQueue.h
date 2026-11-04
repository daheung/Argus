#pragma once

#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Private/Notify/ImageNotifyInfo.h"

class FImageNotifyQueue
{
public:
	explicit FImageNotifyQueue(uint32 Tag) noexcept
		: ImageNotifyInfos(Tag)
		, bInitSucceed(0)
		, TryRestoreCount(0)
	{
		ExInitializePushLock(&this->ImageNotifyInfosLock);

		EContainerStatus InitStatus = this->ImageNotifyInfos.Initialize(this->ImageNotifyMaxInfos);
		if (InitStatus == EContainerStatus::Success)
		{
			this->bInitSucceed.store(1, Atomic::EMemoryOrder::Release);
		}
	}

	FImageNotifyQueue(const FImageNotifyQueue&) = delete;
	FImageNotifyQueue& operator=(const FImageNotifyQueue&) = delete;

	void OnImageNotify(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo);

	bool TryPop(Argus::Notify::FImageNotifyInfo& OutInfo) noexcept
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&this->ImageNotifyInfosLock);

		bool bSuccess = this->ImageNotifyInfos.TryPop(OutInfo);

		ExReleasePushLockExclusive(&this->ImageNotifyInfosLock);
		KeLeaveCriticalRegion();

		return bSuccess;
	}

	uint64 GetDroppedCount() noexcept
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockShared(&this->ImageNotifyInfosLock);

		const uint64 Count = this->DroppedImageNotifyCount;

		ExReleasePushLockShared(&this->ImageNotifyInfosLock);
		KeLeaveCriticalRegion();
		return Count;
	}

private:
	bool TryRestore() noexcept
	{
		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Acquire) != 0)
		{
			return true;
		}

		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&this->ImageNotifyInfosLock);

		if (this->bInitSucceed.load(Atomic::EMemoryOrder::Relaxed) == 0 &&
			this->TryRestoreCount < TryMaxRestoreCount)
		{
			EContainerStatus InitStatus = this->ImageNotifyInfos.Initialize(this->ImageNotifyMaxInfos);
			if (InitStatus == EContainerStatus::Success)
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
			++this->DroppedImageNotifyCount;
		}

		ExReleasePushLockExclusive(&this->ImageNotifyInfosLock);
		KeLeaveCriticalRegion();

		return bReady;
	}

public:
	//static constexpr uint16 ImageNotifyMaxFileNameBytes = Argus::Notify::ImageNotifyMaxFileNameBytes;
	static constexpr uint16 ImageNotifyMaxInfos = 2048;
	static constexpr uint16 TryMaxRestoreCount = 3;

private:
	//TKrnlArray<EPoolType::Paged, Argus::Notify::FImageNotifyInfo> ImageNotifyInfos;
	TSpscCircularQueue<EPoolType::Paged, Argus::Notify::FImageNotifyInfo> ImageNotifyInfos;
	EX_PUSH_LOCK ImageNotifyInfosLock;
	uint64 DroppedImageNotifyCount = 0;

	TKernelAtomicCounter<uint32> bInitSucceed;
	uint32 TryRestoreCount;
};
