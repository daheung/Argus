#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlArray.h"

enum class EIsolationFlags : uint32
{
	None					    =  0u,
	DenyChildProcessCreation	=  1u << 0,
	RestrictProcessHandleRights =  1u << 1,
	RestrictThreadHandleRights  =  1u << 2,

	All							= (1u << 0) | (1u << 1) | (1u << 2)
};

struct FIsolationProcess
{
public:
	FIsolationProcess() noexcept
		: Process(nullptr)
		, Flags(EIsolationFlags::All)
	{}

	FIsolationProcess(PEPROCESS InProcess) noexcept
		: Process(InProcess)
		, Flags(EIsolationFlags::All)
	{}

	FIsolationProcess(PEPROCESS InProcess, EIsolationFlags InFlags) noexcept
		: Process(InProcess)
		, Flags(InFlags)
	{}

public:
	PEPROCESS Process;
	EIsolationFlags Flags;
};

// Tracks isolation membership. Individual callbacks enforce their own policies.
class FIsolationProcessRegistry
{
public:
	FIsolationProcessRegistry() noexcept
		: Processes('IsPk')
	{
		ExInitializePushLock(&this->ProcessesLock);
	}

	~FIsolationProcessRegistry() noexcept;

	FIsolationProcessRegistry(const FIsolationProcessRegistry&) = delete;
	FIsolationProcessRegistry& operator=(const FIsolationProcessRegistry&) = delete;

public:
	bool RegisterProcess(PEPROCESS Process, EIsolationFlags IsolationFlags, bool* bIsExist = nullptr) noexcept;

	bool UnregisterProcess(PEPROCESS Process, bool* bNotFound = nullptr) noexcept;

	bool IsIsolatedProcess(PEPROCESS Process) noexcept;

	bool GetProcessFlags(PEPROCESS Process, EIsolationFlags& OutFlags) noexcept;

private:
	TArray<EPoolType::Paged, FIsolationProcess> Processes;
	EX_PUSH_LOCK ProcessesLock;
};
