#include "Kernel/Private/Notify/ObjectOperationNotifyQueue.h"
#include "Kernel/Private/Notify/ObjectOperationNotifyInfo.h"
#include "Kernel/Private/Callback/ObjectCallback.h"
#include "Core/Public/Memory/Memory.h"

namespace
{
	/**
	 * @brief Gets the requested-access field that a pre-callback can reduce.
	 * @param Information The pending handle create or duplicate operation.
	 * @return The requested-access field, or nullptr for an unsupported operation.
	 */
	ACCESS_MASK GetDesiredAccess(OB_PRE_OPERATION_INFORMATION& Information) noexcept
	{
		switch (Information.Operation)
		{
		case OB_OPERATION_HANDLE_CREATE:
			return Information.Parameters->CreateHandleInformation.DesiredAccess;
		case OB_OPERATION_HANDLE_DUPLICATE:
			return Information.Parameters->DuplicateHandleInformation.DesiredAccess;
		default:
			return 0;
		}
	}

	ACCESS_MASK GetOrignalDesiredAccess(OB_PRE_OPERATION_INFORMATION& Information) noexcept
	{
		switch (Information.Operation)
		{
		case OB_OPERATION_HANDLE_CREATE:
			return Information.Parameters->CreateHandleInformation.OriginalDesiredAccess;
		case OB_OPERATION_HANDLE_DUPLICATE:
			return Information.Parameters->DuplicateHandleInformation.OriginalDesiredAccess;
		default:
			return 0;
		}
	}

	Argus::Notify::EObjectOperationType GetOperationType(const OB_PRE_OPERATION_INFORMATION& Information) noexcept
	{
		switch (Information.Operation)
		{
		case OB_OPERATION_HANDLE_CREATE:
			return Argus::Notify::EObjectOperationType::Create;
		case OB_OPERATION_HANDLE_DUPLICATE:
			return Argus::Notify::EObjectOperationType::Duplicate;
		default:
			return Argus::Notify::EObjectOperationType::Unknown;
		}
	}

	Argus::Notify::EObjectTargetType GetTargetType(const OB_PRE_OPERATION_INFORMATION& Information) noexcept
	{
		if (Information.ObjectType == *PsProcessType)
		{
			return Argus::Notify::EObjectTargetType::Process;
		}
		if (Information.ObjectType == *PsThreadType)
		{
			return Argus::Notify::EObjectTargetType::Thread;
		}

		return Argus::Notify::EObjectTargetType::Unknown;
	}

	PEPROCESS GetHandleOwnerProcess(POB_PRE_OPERATION_INFORMATION Information) noexcept
	{
		if (Information == nullptr || Information->Parameters == nullptr)
		{
			return nullptr;
		}

		switch (Information->Operation)
		{
		case OB_OPERATION_HANDLE_CREATE:
			return PsGetCurrentProcess();
		case OB_OPERATION_HANDLE_DUPLICATE:
			return static_cast<PEPROCESS>(Information->Parameters->DuplicateHandleInformation.TargetProcess);
		default:
			return nullptr;
		}
	}

	PEPROCESS GetSourceProcess(POB_PRE_OPERATION_INFORMATION Information) noexcept
	{
		if (Information == nullptr || Information->Parameters == nullptr)
		{
			return nullptr;
		}

		if (Information->Operation == OB_OPERATION_HANDLE_DUPLICATE)
		{
			return static_cast<PEPROCESS>(Information->Parameters->DuplicateHandleInformation.SourceProcess);
		}

		return nullptr;
	}

	ACCESS_MASK GetGrantedAccess(const OB_POST_OPERATION_INFORMATION& Information) noexcept
	{
		if (!NT_SUCCESS(Information.ReturnStatus))
		{
			return 0;
		}

		switch (Information.Operation)
		{
		case OB_OPERATION_HANDLE_CREATE:
			return Information.Parameters->CreateHandleInformation.GrantedAccess;
		case OB_OPERATION_HANDLE_DUPLICATE:
			return Information.Parameters->DuplicateHandleInformation.GrantedAccess;
		default:
			return 0;
		}
	}

	struct FPendingObjectOperation
	{
		Argus::Notify::FObjectOperationNotifyInfo Info{};
		void* PreviousCallContext = nullptr;
	};
}

void FObjectOperationNotifyQueue::OnPreOperation(POB_PRE_OPERATION_INFORMATION Information, ACCESS_MASK DesiredAccessBeforePolicy) noexcept
{
	if (Information == nullptr || Information->Parameters == nullptr)
	{
		return;
	}
	// Record only operations whose requested rights were reduced by an Argus rule.
	if ((DesiredAccessBeforePolicy & ~GetDesiredAccess(*Information)) == 0)
	{
		return;
	}
	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FObjectOperationNotifyInfo Info{};
	KeQuerySystemTimePrecise(&Info.PreOperationAt);
	Info.OperationType		  = GetOperationType(*Information);
	Info.TargetType			  = GetTargetType(*Information);
	Info.RequesterProcessId	  = HandleToULong(PsGetCurrentProcessId());
	Info.RequesterThreadId	  = HandleToULong(PsGetCurrentThreadId());
	Info.HandleOwnerProcessId = HandleToULong(PsGetProcessId(GetHandleOwnerProcess(Information)));
	
	if (auto SourceProcess = GetSourceProcess(Information); SourceProcess)
	{
		Info.SourceProcessId = HandleToULong(PsGetProcessId(SourceProcess));
	}

	if (Information->ObjectType == *PsProcessType)
	{
		PEPROCESS Process = static_cast<PEPROCESS>(Information->Object);
		Info.TargetProcessId = HandleToULong(PsGetProcessId(Process));
		Info.TargetThreadId  = 0;
	}
	else
	{
		PETHREAD Thread = static_cast<PETHREAD>(Information->Object);
		Info.TargetProcessId = HandleToULong(PsGetThreadProcessId(Thread));
		Info.TargetThreadId  = HandleToULong(PsGetThreadId(Thread));
	}

	Info.OriginalDesiredAccess     = GetOrignalDesiredAccess(*Information);
	Info.DesiredAccessBeforePolicy = DesiredAccessBeforePolicy;
	Info.DesiredAccessAfterPolicy  = GetDesiredAccess(*Information);

	// Post operation
	Info.GrantedAccess = 0;
	Info.ReturnStatus = 0;

	auto* Pending = static_cast<FPendingObjectOperation*>(ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(FPendingObjectOperation), this->PoolTag));
	if (Pending == nullptr)
	{
		++this->DroppedObjectOperationNotifyCount;
		return;
	}

	ConstructItem(Pending);
	Pending->Info = MoveTemp(Info);
	Pending->PreviousCallContext = Information->CallContext;
	Information->CallContext = Pending;
}

void FObjectOperationNotifyQueue::OnPostOperation(POB_POST_OPERATION_INFORMATION Information) noexcept
{
	if (Information == nullptr || Information->CallContext == nullptr ||
		Information->CallContext == FObjectCallback::GetTerminateAccessRemovedContext())
	{
		return;
	}

	auto* Pending = static_cast<FPendingObjectOperation*>(Information->CallContext);
	
	Argus::Notify::FObjectOperationNotifyInfo& Info = Pending->Info;
	KeQuerySystemTimePrecise(&Info.PostOperationAt);
	Info.ReturnStatus  = Information->ReturnStatus;
	Info.GrantedAccess = GetGrantedAccess(*Information);

	this->EnqueueReady(MoveTemp(Pending->Info));

	DestructItem(Pending);
	ExFreePool(Pending);
}

bool FObjectOperationNotifyQueue::HasPreviousCallContext(PVOID CurrentContext, PVOID PreviousContext) const noexcept
{
	if (CurrentContext == nullptr ||
		CurrentContext == FObjectCallback::GetTerminateAccessRemovedContext())
	{
		return false;
	}

	return static_cast<const FPendingObjectOperation*>(CurrentContext)->PreviousCallContext == PreviousContext;
}

void FObjectOperationNotifyQueue::Lock(ELockType LockType) noexcept
{
	KeEnterCriticalRegion();

	if (LockType == ELockType::Shared)
	{
		ExAcquirePushLockShared(&this->ObjectOperationNotifyInfosLock);
	}
	else
	{
		ExAcquirePushLockExclusive(&this->ObjectOperationNotifyInfosLock);
	}
}

void FObjectOperationNotifyQueue::Unlock(ELockType LockType) noexcept
{
	if (LockType == ELockType::Shared)
	{
		ExReleasePushLockShared(&this->ObjectOperationNotifyInfosLock);
	}
	else
	{
		ExReleasePushLockExclusive(&this->ObjectOperationNotifyInfosLock);
	}

	KeLeaveCriticalRegion();
}

uint64 FObjectOperationNotifyQueue::GetDroppedCount() noexcept
{
	KeEnterCriticalRegion();
	ExAcquirePushLockShared(&this->ObjectOperationNotifyInfosLock);
	
	const uint64 Count = this->DroppedObjectOperationNotifyCount;

	ExReleasePushLockShared(&this->ObjectOperationNotifyInfosLock);
	KeLeaveCriticalRegion();

	return Count;
}

void FObjectOperationNotifyQueue::EnqueueReady(Argus::Notify::FObjectOperationNotifyInfo&& Info) noexcept
{
	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ObjectOperationNotifyInfosLock);

	Argus::Notify::FObjectOperationNotifyInfo EvictedInfo{};
	const bool bWasFull = this->ObjectOperationNotifyInfos.IsFull();
	const bool bPushSucceed =
		this->ObjectOperationNotifyInfos.PushDropOldestIfFull(MoveTemp(Info), &EvictedInfo) != INDEX_NONE;
	if (!bPushSucceed || bWasFull)
	{
		++this->DroppedObjectOperationNotifyCount;
	}

	ExReleasePushLockExclusive(&this->ObjectOperationNotifyInfosLock);
	KeLeaveCriticalRegion();
}

bool FObjectOperationNotifyQueue::TryRestore() noexcept
{
	if (this->bInitSucceed.load(Atomic::EMemoryOrder::Acquire) != 0)
	{
		return true;
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ObjectOperationNotifyInfosLock);

	if (this->bInitSucceed.load(Atomic::EMemoryOrder::Relaxed) == 0 &&
		this->TryRestoreCount < TryMaxRestoreCount)
	{
		EContainerStatus InitStatus = this->ObjectOperationNotifyInfos.Initialize(this->ObjectOperationNotifyMaxInfos);
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
		++this->DroppedObjectOperationNotifyCount;
	}

	ExReleasePushLockExclusive(&this->ObjectOperationNotifyInfosLock);
	KeLeaveCriticalRegion();

	return bReady;
}
