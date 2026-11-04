#include "Kernel/Private/Protection/AgentState.h"
#include "Kernel/Public/Containers/KrnlString.h"

extern "C" UCHAR* PsGetProcessImageFileName(PEPROCESS Process);

namespace
{
	// Preliminary name check. Replace with the approved image/candidate check.
	constexpr char ExpectedAgentImageNameLiteral[] = "ArgusAgent.exe";
	static_assert(sizeof(ExpectedAgentImageNameLiteral) <= 15);
}

FAgentState::~FAgentState() noexcept
{
	ClearAgentProcess();
}

NTSTATUS FAgentState::RegisterAgentProcess(FArgusAgentProcess InAgentProcess) noexcept
{
	if (InAgentProcess.Process == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}
	ObReferenceObject(InAgentProcess.Process);

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->AgentProcessLock);

	PEPROCESS PreviousProcess = nullptr;
	NTSTATUS Status = STATUS_SUCCESS;

	Status = this->RegisterAgentProcessCheck(InAgentProcess);
	if (NT_SUCCESS(Status))
	{
		PreviousProcess = this->AgentProcess.Process;
		this->AgentProcess = InAgentProcess;
	}

	ExReleasePushLockExclusive(&this->AgentProcessLock);
	KeLeaveCriticalRegion();

	if (!NT_SUCCESS(Status))
	{
		ObDereferenceObject(InAgentProcess.Process);
		return Status;
	}
	if (PreviousProcess != nullptr)
	{
		ObDereferenceObject(PreviousProcess);
	}
	return STATUS_SUCCESS;
}

void FAgentState::ClearAgentProcess() noexcept
{
	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->AgentProcessLock);

	PEPROCESS PreviousProcess = this->AgentProcess.Process;
	this->AgentProcess = {};

	ExReleasePushLockExclusive(&this->AgentProcessLock);
	KeLeaveCriticalRegion();

	if (PreviousProcess != nullptr)
	{
		ObDereferenceObject(PreviousProcess);
	}
}

NTSTATUS FAgentState::RegisterAgentProcessCheck(const FArgusAgentProcess& InAgentProcess) noexcept
{
	// Caller holds AgentProcessLock exclusively.
	if (InAgentProcess.Process == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	if (this->AgentProcess.Process != nullptr &&
		this->AgentProcess.Process != InAgentProcess.Process)
	{
		return STATUS_DEVICE_BUSY;
	}

	FString<EPoolType::Paged> ImageName; 
	const bool bInitSucceed = ImageName.InitializeFromCString(PsGetProcessImageFileName(InAgentProcess.Process));
	if (!bInitSucceed)
	{
		return STATUS_ACCESS_DENIED;
	}
	if (ImageName.IsEmpty())
	{
		return STATUS_ACCESS_DENIED;
	}

	FString<EPoolType::Paged> ExpectedAgentImageName;
	if (!ExpectedAgentImageName.InitializeFromCString(ExpectedAgentImageNameLiteral))
	{
		return STATUS_ACCESS_DENIED;
	}

	// Compare the complete short image name, including its terminator.
	if (ImageName.Compare(ExpectedAgentImageName, ESearchCase::CaseSecsitive) != 0)
	{
		return STATUS_ACCESS_DENIED;
	}

	return STATUS_SUCCESS;
}

void FAgentState::OnProcessExit(PEPROCESS Process) noexcept
{
	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->AgentProcessLock);

	const bool bAgentExited = this->AgentProcess.Process == Process;
	if (bAgentExited)
	{
		this->AgentProcess = {};
	}

	ExReleasePushLockExclusive(&this->AgentProcessLock);
	KeLeaveCriticalRegion();

	if (bAgentExited)
	{
		ObDereferenceObject(Process);
	}
}

bool FAgentState::IsAgentProcess(PEPROCESS Process) noexcept
{
	KeEnterCriticalRegion();
	ExAcquirePushLockShared(&this->AgentProcessLock);

	const bool bIsAgentProcess = this->AgentProcess.Process != nullptr && this->AgentProcess.Process == Process;
	
	ExReleasePushLockShared(&this->AgentProcessLock);
	KeLeaveCriticalRegion();

	return bIsAgentProcess;
}
