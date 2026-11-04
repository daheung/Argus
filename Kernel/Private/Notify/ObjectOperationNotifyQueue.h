#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Private/Notify/ObjectOperationNotifyInfo.h"
#include "Kernel/Private/Notify/LockType.h"

class FObjectOperationNotifyQueue
{
public:
	typedef TSpscCircularQueue<EPoolType::Paged, Argus::Notify::FObjectOperationNotifyInfo> TQueue;

public:
	explicit FObjectOperationNotifyQueue(uint32 Tag) noexcept
		: ObjectOperationNotifyInfos(Tag)
		, ObjectOperationNotifyInfosLock()
		, DroppedObjectOperationNotifyCount(0)
		, PoolTag(Tag)
		, bInitSucceed(0)
		, TryRestoreCount(0)
	{
		ExInitializePushLock(&this->ObjectOperationNotifyInfosLock);
		if (this->ObjectOperationNotifyInfos.Initialize(ObjectOperationNotifyMaxInfos) == EContainerStatus::Success)
		{
			this->bInitSucceed.store(1, Atomic::EMemoryOrder::Release);
		}
	}

	FObjectOperationNotifyQueue(const FObjectOperationNotifyQueue&) = delete;
	FObjectOperationNotifyQueue& operator=(const FObjectOperationNotifyQueue&) = delete;

public:
	void OnPreOperation(POB_PRE_OPERATION_INFORMATION Information, ACCESS_MASK DesiredAccessBeforePolicy) noexcept;

	void OnPostOperation(POB_POST_OPERATION_INFORMATION Information) noexcept;
	bool HasPreviousCallContext(PVOID CurrentContext, PVOID PreviousContext) const noexcept;


public:
	NODISCARD FORCEINLINE TQueue& GetQueue() noexcept
	{
		return this->ObjectOperationNotifyInfos;
	}

	void Lock(ELockType LockType) noexcept;

	void Unlock(ELockType LockType) noexcept;

public:
	uint64 GetDroppedCount() noexcept;

	static constexpr uint16 ObjectOperationNotifyMaxInfos = 2048;
	static constexpr uint16 TryMaxRestoreCount = 3;

private:
	void EnqueueReady(Argus::Notify::FObjectOperationNotifyInfo&& Info) noexcept;
	
	bool TryRestore() noexcept;

	TQueue ObjectOperationNotifyInfos;
	EX_PUSH_LOCK ObjectOperationNotifyInfosLock;
	uint64 DroppedObjectOperationNotifyCount = 0;
	uint32 PoolTag;

	TKernelAtomicCounter<uint32> bInitSucceed;
	uint32 TryRestoreCount;
};
