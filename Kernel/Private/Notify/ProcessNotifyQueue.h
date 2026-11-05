#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Private/Notify/ProcessNotifyInfo.h"
#include "Kernel/Private/Notify/LockType.h"

class FProcessNotifyQueue
{
public:
	typedef TSpscCircularQueue<EPoolType::Paged, Argus::Notify::FProcessNotifyInfo> TQueue;

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

public:
	void OnProcessCreateNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo, const Argus::Notify::FProcessNotifyPolicy& InProcessPolicy);

	void OnProcessExitNotify(PEPROCESS Process, HANDLE ProcessId);

	bool TryPop(Argus::Notify::FProcessNotifyInfo& OutInfo) noexcept;

	uint64 GetDroppedCount() noexcept;

public:
	NODISCARD FORCEINLINE TQueue& GetQueue() noexcept
	{
		return this->ProcessNotifyInfos;
	}

	FORCEINLINE void Lock(ELockType LockType) noexcept
	{
		KeEnterCriticalRegion();

		if (LockType == ELockType::Shared)
		{
			ExAcquirePushLockShared(&this->ProcessNotifyInfosLock);
		}
		else
		{
			ExAcquirePushLockExclusive(&this->ProcessNotifyInfosLock);
		}
	}

	FORCEINLINE void Unlock(ELockType LockType) noexcept
	{
		if (LockType == ELockType::Shared)
		{
			ExReleasePushLockShared(&this->ProcessNotifyInfosLock);
		}
		else
		{
			ExReleasePushLockExclusive(&this->ProcessNotifyInfosLock);
		}

		KeLeaveCriticalRegion();
	}

public:
	static constexpr uint16 ProcessNotifyMaxInfos = 1024;
	static constexpr uint16 TryMaxRestoreCount = 3;

private:
	bool TryRestore() noexcept;

private:
	TQueue ProcessNotifyInfos;
	EX_PUSH_LOCK ProcessNotifyInfosLock;
	uint64 DroppedProcessNotifyCount = 0;

	TKernelAtomicCounter<uint32> bInitSucceed;
	uint32 TryRestoreCount;
};
