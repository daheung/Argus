#include "Kernel/Private/Notify/ProcessNotifyQueue.h"

bool FProcessNotifyQueue::TryPop(Argus::Notify::FProcessNotifyInfo& OutInfo) noexcept
{
	return this->ProcessNotifyInfos.TryPop(OutInfo);
}

void FProcessNotifyQueue::OnProcessCreateNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo, const Argus::Notify::FProcessNotifyPolicy& ProcessPolicy)
{
	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FProcessNotifyInfo ProcessInfo{};

	ProcessInfo.CreatedAt.QuadPart = PsGetProcessCreateTimeQuadPart(Process);
	ProcessInfo.NotifyType		   = Argus::Notify::EProcessNotifyType::Create;
	ProcessInfo.ProcessId		   = HandleToULong(ProcessId);
	ProcessInfo.ParentProcessId	   = HandleToULong(CreateInfo->ParentProcessId);
	ProcessInfo.CreatingProcessId  = HandleToULong(CreateInfo->CreatingThreadId.UniqueProcess);
	ProcessInfo.CreatingThreadId   = HandleToULong(CreateInfo->CreatingThreadId.UniqueThread);
	ProcessInfo.CreationStatus	   = CreateInfo->CreationStatus;
	
	ProcessInfo.PolicyRole	 = ProcessPolicy.PolicyRole;
	ProcessInfo.PolicyStatus = ProcessPolicy.PolicyStatus;

	PCUNICODE_STRING CommandLine = CreateInfo->CommandLine;
	if (CommandLine != nullptr && CommandLine->Buffer != nullptr &&
		CommandLine->Length / sizeof(FString<>::ElementType))
	{
		ProcessInfo.CommandLine.InitializeWithLength(
			CommandLine->Buffer, CommandLine->Length / sizeof(FString<>::ElementType));
	}

	PCUNICODE_STRING ImageFileName = CreateInfo->ImageFileName;
	if (ImageFileName != nullptr && ImageFileName->Buffer != nullptr && 
		ImageFileName->Length / sizeof(FString<>::ElementType))
	{
		const uint16 ImageChars = ImageFileName->Length / sizeof(WCHAR);
		FString<> ImagePath;
		if (ImagePath.InitializeWithLength(ImageFileName->Buffer, ImageChars))
		{
			ImagePath.ReplaceAll(L'/', L'\\', nullptr);

			int32 LastSeparator = INDEX_NONE;
			const int32 BaseNameStart = ImagePath.FindLastChar(L'\\', LastSeparator) ? LastSeparator + 1 : 0;
			if (BaseNameStart != 0)
			{
				ImagePath.RightChopInline(BaseNameStart);
			}

			FString<>::Swap(ImagePath, ProcessInfo.ImageFileName);
		}
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ProcessNotifyInfosLock);

	Argus::Notify::FProcessNotifyInfo EvictedInfo{};
	const bool bWasFull = this->ProcessNotifyInfos.IsFull();
	const bool bPushSucceed =
		this->ProcessNotifyInfos.PushDropOldestIfFull(MoveTemp(ProcessInfo), &EvictedInfo) != INDEX_NONE;
	if (!bPushSucceed || bWasFull)
	{
		++this->DroppedProcessNotifyCount;
	}

	ExReleasePushLockExclusive(&this->ProcessNotifyInfosLock);
	KeLeaveCriticalRegion();
}

void FProcessNotifyQueue::OnProcessExitNotify(PEPROCESS Process, HANDLE ProcessId)
{
	if (!this->TryRestore())
	{
		return;
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ProcessNotifyInfosLock);

	int32 CreateInfoIndex = this->ProcessNotifyInfos.FindIndexByPredicate([ProcessId](const Argus::Notify::FProcessNotifyInfo& Info) -> bool { 
		return Info.ProcessId == HandleToULong(ProcessId); 
	});

	const bool bFound = CreateInfoIndex != INDEX_NONE;
	if (bFound)
	{
		Argus::Notify::FProcessNotifyInfo& CreateInfo = this->ProcessNotifyInfos.At(CreateInfoIndex);
		KeQuerySystemTimePrecise(&CreateInfo.ExitedAt);
		CreateInfo.NotifyType = Argus::Notify::EProcessNotifyType::Both;
		CreateInfo.ExitCode	  = PsGetProcessExitStatus(Process);
	}
	else
	{
		Argus::Notify::FProcessNotifyInfo ExitInfo{};
		KeQuerySystemTimePrecise(&ExitInfo.ExitedAt);

		ExitInfo.NotifyType = Argus::Notify::EProcessNotifyType::Exit;
		ExitInfo.ProcessId  = HandleToULong(ProcessId);
		ExitInfo.ExitCode   = PsGetProcessExitStatus(Process);

		Argus::Notify::FProcessNotifyInfo EvictedInfo{};
		const bool bWasFull = this->ProcessNotifyInfos.IsFull();
		const bool bPushSucceed =
			this->ProcessNotifyInfos.PushDropOldestIfFull(MoveTemp(ExitInfo), &EvictedInfo) != INDEX_NONE;
		if (!bPushSucceed || bWasFull)
		{
			++this->DroppedProcessNotifyCount;
		}
	}

	ExReleasePushLockExclusive(&this->ProcessNotifyInfosLock);
	KeLeaveCriticalRegion();
}

uint64 FProcessNotifyQueue::GetDroppedCount() noexcept
{
	KeEnterCriticalRegion();
	ExAcquirePushLockShared(&this->ProcessNotifyInfosLock);

	const uint64 Count = this->DroppedProcessNotifyCount;

	ExReleasePushLockShared(&this->ProcessNotifyInfosLock);
	KeLeaveCriticalRegion();

	return Count;
}

bool FProcessNotifyQueue::TryRestore() noexcept
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