#include "Kernel/Private/Notify/ProcessNotifyQueue.h"

void FProcessNotifyQueue::OnProcessCreateNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FProcessNotifyInfo ProcessInfo{};
	ProcessInfo.CreatedAt.QuadPart = PsGetProcessCreateTimeQuadPart(Process);

	const bool bCreationSucceed = NT_SUCCESS(CreateInfo->CreationStatus);
	if (bCreationSucceed)
	{
		ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::Created;
	}
	else
	{
		if (CreateInfo->CreationStatus == STATUS_ACCESS_DENIED)
		{
			ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::CreationDenied;
		}
		else
		{
			ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::CreationFailed;
		}
	}

	ProcessInfo.ProcessId		  = HandleToULong(ProcessId);
	ProcessInfo.ParentProcessId   = HandleToULong(CreateInfo->ParentProcessId);
	ProcessInfo.CreatingProcessId = HandleToULong(CreateInfo->CreatingThreadId.UniqueProcess);
	ProcessInfo.CreatingThreadId  = HandleToULong(CreateInfo->CreatingThreadId.UniqueThread);
	ProcessInfo.CreationStatus	  = CreateInfo->CreationStatus;

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

	Argus::Notify::FProcessNotifyInfo ProcessInfo{};
	KeQuerySystemTimePrecise(&ProcessInfo.ExitedAt);

	ProcessInfo.NotifyType = Argus::Notify::EProcessNotifyType::Exited;
	ProcessInfo.ProcessId  = HandleToULong(ProcessId);
	ProcessInfo.ExitCode   = PsGetProcessExitStatus(Process);

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
