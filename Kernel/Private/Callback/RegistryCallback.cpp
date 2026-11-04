#include "Kernel/Private/Callback/RegistryCallback.h"


NTSTATUS FRegistryCallback::HandleRegistryCallback(FDriverContext& Context, void* ArgumentA, void* ArgumentB) noexcept
{
	const FRegistryOperationContext Operation = BuildRegistryOperationContext(Context, ArgumentA, ArgumentB);

	if (Operation.Phase == Argus::Notify::ERegistryNotifyPhase::Pre)
	{
		return this->OnPreRegistryCallback(Context, Operation);
	}
	if (Operation.Phase == Argus::Notify::ERegistryNotifyPhase::Post)
	{
		this->OnPostRegistryCallback(Operation);
	}
	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCallback::OnPreRegistryCallback(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept
{
	using Argus::Notify::ERegistryOperationType;

	// Operation-specific helpers will decode Information according to NotifyClass.
	// Each helper can then apply all matching requester roles without an else-if chain.
	switch (Operation.OperationType)
	{
	case ERegistryOperationType::CreateKey:
		return FRegistryCreateKeyRules::ApplyRule(Context, Operation);
	case ERegistryOperationType::SetKeyInformation:
		return this->ApplySetKeyInformationRule(Operation);
	case ERegistryOperationType::SetValue:
		return this->ApplySetValueRule(Operation);
	case ERegistryOperationType::DeleteKey:
		return this->ApplyDeleteKeyRule(Operation);
	case ERegistryOperationType::DeleteValue:
		return this->ApplyDeleteValueRule(Operation);
	default:
		return STATUS_SUCCESS;
	}
}

void FRegistryCallback::OnPostRegistryCallback(const FRegistryOperationContext& Operation) noexcept
{
	// The notify queue records completion status. Post policy is not part of
	// this audit-only draft.
	UNREFERENCED_PARAMETER(Operation);
}

NTSTATUS FRegistryCallback::ApplySetKeyInformationRule(const FRegistryOperationContext& Operation) noexcept
{
	UNREFERENCED_PARAMETER(Operation);
	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCallback::ApplySetValueRule(const FRegistryOperationContext& Operation) noexcept
{
	UNREFERENCED_PARAMETER(Operation);
	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCallback::ApplyDeleteKeyRule(const FRegistryOperationContext& Operation) noexcept
{
	UNREFERENCED_PARAMETER(Operation);
	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCallback::ApplyDeleteValueRule(const FRegistryOperationContext& Operation) noexcept
{
	UNREFERENCED_PARAMETER(Operation);
	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCreateKeyRules::ApplyRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept
{
	switch (Operation.RequesterRoles)
	{
	case Argus::Notify::ERegistryRequesterRole::Agent:
		return FRegistryCreateKeyRules::ApplyAgentProcessRule(Context, Operation);
	case Argus::Notify::ERegistryRequesterRole::Protected:
		return FRegistryCreateKeyRules::ApplyProtectedProcessRule(Context, Operation);
	case Argus::Notify::ERegistryRequesterRole::Isolated:
		return FRegistryCreateKeyRules::ApplyIsolationProcessRule(Context, Operation);
	default:
		break;
	} 

	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCreateKeyRules::ApplyAgentProcessRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Operation);
	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCreateKeyRules::ApplyProtectedProcessRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Operation);
	return STATUS_SUCCESS;
}

NTSTATUS FRegistryCreateKeyRules::ApplyIsolationProcessRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Operation);
	return STATUS_SUCCESS;
}
