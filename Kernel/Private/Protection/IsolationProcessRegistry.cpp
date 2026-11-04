#include "Kernel/Private/Protection/IsolationProcessRegistry.h"

FIsolationProcessRegistry::~FIsolationProcessRegistry() noexcept
{
	for (int32 Index = 0; Index < this->Processes.Num(); ++Index)
	{
		ObDereferenceObject(this->Processes[Index].Process);
	}
}

bool FIsolationProcessRegistry::RegisterProcess(PEPROCESS Process, EIsolationFlags IsolationFlags, bool* bIsExist) noexcept
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

	const int32 Index = this->Processes.FindIndexByPredicate([Process](const FIsolationProcess& Data) -> bool {
		return Data.Process == Process;
	});
	if (Index == INDEX_NONE)
	{
		bSucceed = this->Processes.Push(FIsolationProcess{ Process, IsolationFlags }) != INDEX_NONE;
		if (bSucceed)
		{
			ObReferenceObject(Process);
		}
	}
	else if (bIsExist)
	{
		*bIsExist = true;
	}

	ExReleasePushLockExclusive(&this->ProcessesLock);
	KeLeaveCriticalRegion();

	return bSucceed;
}

bool FIsolationProcessRegistry::UnregisterProcess(PEPROCESS Process, bool* bNotFound) noexcept
{
	if (bNotFound)
	{
		*bNotFound = false;
	}
	if (Process == nullptr)
	{
		return false;
	}

	FIsolationProcess RemovedProcess{};

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ProcessesLock);

	const int32 Index = this->Processes.FindIndexByPredicate([Process](const FIsolationProcess& Data) -> bool {
		return Data.Process == Process;
	});
	const bool bRemoved = Index != INDEX_NONE && this->Processes.PopAt(Index, RemovedProcess);
	if (Index == INDEX_NONE && bNotFound)
	{
		*bNotFound = true;
	}

	ExReleasePushLockExclusive(&this->ProcessesLock);
	KeLeaveCriticalRegion();

	if (bRemoved)
	{
		CORE_CHECK(RemovedProcess.Process != nullptr);
		ObDereferenceObject(RemovedProcess.Process);
	}

	return bRemoved;
}

bool FIsolationProcessRegistry::IsIsolatedProcess(PEPROCESS Process) noexcept
{
	if (Process == nullptr)
	{
		return false;
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockShared(&this->ProcessesLock);

	const bool bFound = this->Processes.FindIndexByPredicate([Process](const FIsolationProcess& Data) -> bool {
		return Data.Process == Process;
	}) != INDEX_NONE;

	ExReleasePushLockShared(&this->ProcessesLock);
	KeLeaveCriticalRegion();

	return bFound;
}

bool FIsolationProcessRegistry::GetProcessFlags(PEPROCESS Process, EIsolationFlags& Flags) noexcept
{
	if (Process == nullptr)
	{
		return false;
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockShared(&this->ProcessesLock);

	const int32 Index = this->Processes.FindIndexByPredicate([Process](const FIsolationProcess& Data) -> bool {
		return Data.Process == Process;
	});
	const bool bFound = Index != INDEX_NONE;
	if (bFound)
	{
		Flags = this->Processes[Index].Flags;
	}

	ExReleasePushLockShared(&this->ProcessesLock);
	KeLeaveCriticalRegion();

	return bFound;
}
