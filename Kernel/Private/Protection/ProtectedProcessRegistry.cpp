#include "Kernel/Private/Protection/ProtectedProcessRegistry.h"

FProtectedProcessRegistry::~FProtectedProcessRegistry() noexcept
{
	for (int32 Index = 0; Index < this->Processes.Num(); ++Index)
	{
		ObDereferenceObject(this->Processes[Index].Process);
	}
}

bool FProtectedProcessRegistry::RegisterProcess(PEPROCESS Process, bool* bIsExist) noexcept
{
	if (bIsExist)
	{
		*bIsExist = false;
	}
	if (Process == nullptr)
	{
		return false;
	}

	bool bSucceed = false;

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ProcessesLock);

	int32 Index = this->Processes.FindIndexByPredicate([Process](const FProtectedProcess& Data) -> bool {
		return Data.Process == Process; 
	});
	if (Index == INDEX_NONE)
	{
		bool bPushSucceed = this->Processes.Push(FProtectedProcess{ Process }) != INDEX_NONE;
		if (bPushSucceed)
		{
			bSucceed = true;
		}
	}
	else
	{
		if (bIsExist)
		{
			*bIsExist = true;
		}
	}

	if (bSucceed)
	{
		ObReferenceObject(Process);
	}

	ExReleasePushLockExclusive(&this->ProcessesLock);
	KeLeaveCriticalRegion();

	return bSucceed;
}

bool FProtectedProcessRegistry::UnregisterProcess(PEPROCESS Process, bool* bNotFound = nullptr) noexcept
{
	if (bNotFound)
	{
		*bNotFound = false;
	}
	if (Process == nullptr)
	{
		return false;
	}

	bool bSucceed = false;
	FProtectedProcess RemovedProcess{};

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ProcessesLock);
	
	int32 Index = this->Processes.FindIndexByPredicate([Process](const FProtectedProcess& Data) -> bool {
		return Data.Process == Process;
	});
	if (Index != INDEX_NONE)
	{
		bool bPopSucceed = this->Processes.PopAt(Index, RemovedProcess);
		if (bPopSucceed)
		{
			bSucceed = true;
		}
	}
	else
	{
		if (bNotFound)
		{
			*bNotFound = true;
		}
	}

	ExReleasePushLockExclusive(&this->ProcessesLock);
	KeLeaveCriticalRegion();

	if (bSucceed)
	{
		CORE_CHECK(RemovedProcess.Process != nullptr);
		ObDereferenceObject(RemovedProcess.Process);
	}

	return bSucceed;
}

bool FProtectedProcessRegistry::IsProtectedProcess(PEPROCESS Process) noexcept
{
	if (Process == nullptr)
	{
		return false;
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockShared(&this->ProcessesLock);

	const bool bIsProtected = this->Processes.FindIndexByPredicate([Process](const FProtectedProcess& Data) -> bool {
		return Data.Process == Process;
	}) != INDEX_NONE;

	ExReleasePushLockShared(&this->ProcessesLock);
	KeLeaveCriticalRegion();

	return bIsProtected;
}
