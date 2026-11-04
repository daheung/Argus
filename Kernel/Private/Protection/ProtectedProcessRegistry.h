#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlArray.h"

struct FProtectedProcess
{
	PEPROCESS Process;
};

class FProtectedProcessRegistry
{
public:
	FProtectedProcessRegistry() noexcept
		: Processes('PtPk')
	{
		ExInitializePushLock(&this->ProcessesLock);
	}

	~FProtectedProcessRegistry() noexcept;

	FProtectedProcessRegistry(const FProtectedProcessRegistry&) = delete;
	FProtectedProcessRegistry& operator=(const FProtectedProcessRegistry&) = delete;

public:
	bool RegisterProcess(PEPROCESS Process, bool* bIsExist) noexcept;
	bool UnregisterProcess(PEPROCESS Process, bool* bNotFound) noexcept;
	bool IsProtectedProcess(PEPROCESS Process) noexcept;

private:
	TArray<EPoolType::Paged, FProtectedProcess> Processes;
	EX_PUSH_LOCK ProcessesLock;
};
