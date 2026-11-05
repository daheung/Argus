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
	NTSTATUS Status = this->RegisterAgentProcessCheck(InAgentProcess);

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

NTSTATUS FAgentState::RegisterAgentProcessCheck(const FArgusAgentProcess& InAgentProcess) const noexcept
{
	const bool bAlreadyExistAgent = this->IsValidAgent();
	if (bAlreadyExistAgent)
	{
		return STATUS_ACCESS_DENIED;
	}

	NTSTATUS CheckName = this->RegisterAgentProcessCheckName(InAgentProcess);
	if (!NT_SUCCESS(CheckName))
	{
		return CheckName;
	}

	NTSTATUS CheckVerify = this->RegisterAgentProcessCheckVerify(InAgentProcess);
	if (!NT_SUCCESS(CheckVerify))
	{
		return CheckVerify;
	}

	return STATUS_SUCCESS;
}

NTSTATUS FAgentState::RegisterAgentProcessCheckName(const FArgusAgentProcess& InAgentProcess) const noexcept
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

NTSTATUS FAgentState::RegisterAgentProcessCheckVerify(const FArgusAgentProcess& InAgentProcess) const noexcept
{
	if (InAgentProcess.Process == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	// S-1-5-80-1711714136-669378850-3648073523-2394855389-1004444190
	// (NT SERVICE\ArgusAgent). Keep these values in sync with ArgusIdentity.h.
	alignas(SID) UCHAR ExpectedSidBuffer[SECURITY_MAX_SID_SIZE]{};
	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID ExpectedSid = reinterpret_cast<PSID>(ExpectedSidBuffer);

	NTSTATUS Status = RtlInitializeSidEx(ExpectedSid, &NtAuthority, 6, 80u, 1711714136u, 669378850u, 3648073523u, 2394855389u, 1004444190u);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	PACCESS_TOKEN AccessToken = PsReferencePrimaryToken(InAgentProcess.Process);
	if (AccessToken == nullptr)
	{
		return STATUS_ACCESS_DENIED;
	}

	void* TokenInformation = nullptr;
	Status = SeQueryInformationToken(AccessToken, TokenGroups, &TokenInformation);
	PsDereferencePrimaryToken(AccessToken);
	if (!NT_SUCCESS(Status))
	{
		if (TokenInformation != nullptr)
		{
			ExFreePool(TokenInformation);
		}
		return Status;
	}

	if (TokenInformation == nullptr)
	{
		return STATUS_UNSUCCESSFUL;
	}

	const auto* Groups = static_cast<const TOKEN_GROUPS*>(TokenInformation);
	constexpr ULONG DenyOnlyGroupFlag = 0x00000010UL;
	bool bServiceSidFound = false;
	for (ULONG Index = 0; Index < Groups->GroupCount; ++Index)
	{
		const auto& Group = Groups->Groups[Index];
		if (Group.Sid != nullptr &&
			(Group.Attributes & DenyOnlyGroupFlag) == 0 &&
			RtlValidSid(Group.Sid) &&
			RtlEqualSid(Group.Sid, ExpectedSid))
		{
			bServiceSidFound = true;
			break;
		}
	}

	ExFreePool(TokenInformation);
	return bServiceSidFound 
		? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
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
