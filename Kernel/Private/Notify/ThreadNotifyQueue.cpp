#include "ThreadNotifyQueue.h"

void FThreadNotifyQueue::OnThreadCreateNotify(HANDLE ProcessId, HANDLE ThreadId)
{
	this->OnThreadNotifyImpl(ProcessId, ThreadId, true);
}

void FThreadNotifyQueue::OnThreadExitNotify(HANDLE ProcessId, HANDLE ThreadId)
{
	this->OnThreadNotifyImpl(ProcessId, ThreadId, false);
}

void FThreadNotifyQueue::OnThreadNotifyImpl(HANDLE ProcessId, HANDLE ThreadId, bool bCreate) noexcept
{
	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FThreadNotifyInfo ThreadInfo{};

	LARGE_INTEGER* At = bCreate
		? &ThreadInfo.CreatedAt
		: &ThreadInfo.ExitedAt;
	KeQuerySystemTimePrecise(At);

	ThreadInfo.NotifyType = bCreate 
		? Argus::Notify::EThreadNotifyType::Created
		: Argus::Notify::EThreadNotifyType::Exited;
	ThreadInfo.ProcessId = HandleToULong(ProcessId);
	ThreadInfo.ThreadId  = HandleToULong(ThreadId);

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ThreadNotifyInfosLock);

	const bool bWasFull = this->ThreadNotifyInfos.IsFull();
	const bool bPushSucceed =
		this->ThreadNotifyInfos.PushDropOldestIfFull(MoveTemp(ThreadInfo)) != INDEX_NONE;
	if (!bPushSucceed || bWasFull)
	{
		++this->DroppedThreadNotifyCount;
	}

	ExReleasePushLockExclusive(&this->ThreadNotifyInfosLock);
	KeLeaveCriticalRegion();
}
