#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"

struct FArgusAgentProcess
{
	PEPROCESS Process;
	uint32 MajorVersion;
	uint32 MinorVersion;
	uint32 PatchVersion;
};

class FAgentState
{
public:
	FAgentState() noexcept
		: AgentProcess()
	{
		ExInitializePushLock(&this->AgentProcessLock);
	}

	~FAgentState() noexcept;

	FAgentState(const FAgentState&) = delete;
	FAgentState& operator=(const FAgentState&) = delete;

	NTSTATUS RegisterAgentProcess(FArgusAgentProcess InAgentProcess) noexcept;
	void OnProcessExit(PEPROCESS Process) noexcept;
	bool IsAgentProcess(PEPROCESS Process) noexcept;

	FORCEINLINE bool IsValidAgent() const noexcept
	{
		return this->AgentProcess.Process != nullptr;
	}

private:
	NTSTATUS RegisterAgentProcessCheck(const FArgusAgentProcess& InAgentProcess) noexcept;
	void ClearAgentProcess() noexcept;

	FArgusAgentProcess AgentProcess;
	EX_PUSH_LOCK AgentProcessLock;
};
