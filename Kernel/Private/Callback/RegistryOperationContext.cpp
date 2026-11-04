#include "Kernel/Private/Callback/RegistryOperationContext.h"
#include "Kernel/Private/DriverContext.h"

static Argus::Notify::ERegistryNotifyPhase GetRegistryNotificationPhase(REG_NOTIFY_CLASS NotifyClass) noexcept
{
	switch (NotifyClass)
	{
	case RegNtPreDeleteKey:
	case RegNtPreSetValueKey:
	case RegNtPreDeleteValueKey:
	case RegNtPreSetInformationKey:
	case RegNtPreRenameKey:
	case RegNtPreEnumerateKey:
	case RegNtPreEnumerateValueKey:
	case RegNtPreQueryKey:
	case RegNtPreQueryValueKey:
	case RegNtPreQueryMultipleValueKey:
	case RegNtPreCreateKey:
	case RegNtPreOpenKey:
	case RegNtPreKeyHandleClose:
	case RegNtPreCreateKeyEx:
	case RegNtPreOpenKeyEx:
	case RegNtPreFlushKey:
	case RegNtPreLoadKey:
	case RegNtPreUnLoadKey:
	case RegNtPreQueryKeySecurity:
	case RegNtPreSetKeySecurity:
	case RegNtPreRestoreKey:
	case RegNtPreSaveKey:
	case RegNtPreReplaceKey:
	case RegNtPreQueryKeyName:
	case RegNtPreSaveMergedKey:
		return Argus::Notify::ERegistryNotifyPhase::Pre;

	case RegNtPostCreateKey:
	case RegNtPostOpenKey:
	case RegNtPostDeleteKey:
	case RegNtPostSetValueKey:
	case RegNtPostDeleteValueKey:
	case RegNtPostSetInformationKey:
	case RegNtPostRenameKey:
	case RegNtPostEnumerateKey:
	case RegNtPostEnumerateValueKey:
	case RegNtPostQueryKey:
	case RegNtPostQueryValueKey:
	case RegNtPostQueryMultipleValueKey:
	case RegNtPostKeyHandleClose:
	case RegNtPostCreateKeyEx:
	case RegNtPostOpenKeyEx:
	case RegNtPostFlushKey:
	case RegNtPostLoadKey:
	case RegNtPostUnLoadKey:
	case RegNtPostQueryKeySecurity:
	case RegNtPostSetKeySecurity:
	case RegNtPostRestoreKey:
	case RegNtPostSaveKey:
	case RegNtPostReplaceKey:
	case RegNtPostQueryKeyName:
	case RegNtPostSaveMergedKey:
		return Argus::Notify::ERegistryNotifyPhase::Post;

	case RegNtCallbackObjectContextCleanup:
	default:
		return Argus::Notify::ERegistryNotifyPhase::Unknown;
	}
}

static Argus::Notify::ERegistryOperationType GetRegistryOperationType(REG_NOTIFY_CLASS NotifyClass) noexcept
{
	using Argus::Notify::ERegistryOperationType;
	switch (NotifyClass)
	{
	case RegNtPreCreateKey:
	case RegNtPreCreateKeyEx:
	case RegNtPostCreateKey:
	case RegNtPostCreateKeyEx:
		return ERegistryOperationType::CreateKey;
	case RegNtPreOpenKey:
	case RegNtPreOpenKeyEx:
	case RegNtPostOpenKey:
	case RegNtPostOpenKeyEx:
		return ERegistryOperationType::OpenKey;
	case RegNtPreDeleteKey:
	case RegNtPostDeleteKey:
		return ERegistryOperationType::DeleteKey;
	case RegNtPreRenameKey:
	case RegNtPostRenameKey:
		return ERegistryOperationType::RenameKey;
	case RegNtPreSetValueKey:
	case RegNtPostSetValueKey:
		return ERegistryOperationType::SetValue;
	case RegNtPreDeleteValueKey:
	case RegNtPostDeleteValueKey:
		return ERegistryOperationType::DeleteValue;
	case RegNtPreQueryKey:
	case RegNtPostQueryKey:
		return ERegistryOperationType::QueryKey;
	case RegNtPreQueryValueKey:
	case RegNtPostQueryValueKey:
		return ERegistryOperationType::QueryValue;
	case RegNtPreQueryMultipleValueKey:
	case RegNtPostQueryMultipleValueKey:
		return ERegistryOperationType::QueryMultipleValues;
	case RegNtPreEnumerateKey:
	case RegNtPostEnumerateKey:
		return ERegistryOperationType::EnumerateKey;
	case RegNtPreEnumerateValueKey:
	case RegNtPostEnumerateValueKey:
		return ERegistryOperationType::EnumerateValue;
	case RegNtPreSetInformationKey:
	case RegNtPostSetInformationKey:
		return ERegistryOperationType::SetKeyInformation;
	case RegNtPreQueryKeySecurity:
	case RegNtPostQueryKeySecurity:
		return ERegistryOperationType::QueryKeySecurity;
	case RegNtPreSetKeySecurity:
	case RegNtPostSetKeySecurity:
		return ERegistryOperationType::SetKeySecurity;
	case RegNtPreFlushKey:
	case RegNtPostFlushKey:
		return ERegistryOperationType::FlushKey;
	case RegNtPreLoadKey:
	case RegNtPostLoadKey:
		return ERegistryOperationType::LoadKey;
	case RegNtPreUnLoadKey:
	case RegNtPostUnLoadKey:
		return ERegistryOperationType::UnloadKey;
	case RegNtPreRestoreKey:
	case RegNtPostRestoreKey:
		return ERegistryOperationType::RestoreKey;
	case RegNtPreSaveKey:
	case RegNtPostSaveKey:
		return ERegistryOperationType::SaveKey;
	case RegNtPreReplaceKey:
	case RegNtPostReplaceKey:
		return ERegistryOperationType::ReplaceKey;
	case RegNtPreKeyHandleClose:
	case RegNtPostKeyHandleClose:
		return ERegistryOperationType::CloseKey;
	case RegNtPreQueryKeyName:
	case RegNtPostQueryKeyName:
		return ERegistryOperationType::QueryKeyName;
	case RegNtPreSaveMergedKey:
	case RegNtPostSaveMergedKey:
		return ERegistryOperationType::SaveMergedKey;
	default:
		return ERegistryOperationType::Unknown;
	}
}

FRegistryOperationContext BuildRegistryOperationContext(FDriverContext& Context,
	void* ArgumentA, void* ArgumentB) noexcept
{
	FRegistryOperationContext Operation{};
	Operation.NotifyClass = static_cast<REG_NOTIFY_CLASS>(reinterpret_cast<ULONG_PTR>(ArgumentA));
	Operation.Phase = GetRegistryNotificationPhase(Operation.NotifyClass);
	if (Operation.Phase == Argus::Notify::ERegistryNotifyPhase::Unknown)
	{
		return Operation;
	}

	Operation.OperationType = GetRegistryOperationType(Operation.NotifyClass);
	Operation.ProcessId = HandleToULong(PsGetCurrentProcessId());
	Operation.ThreadId = HandleToULong(PsGetCurrentThreadId());
	Operation.Information = ArgumentB;

	// Classify the thread owner rather than an optionally attached address space.
	PEPROCESS RequesterProcess = nullptr;

	const NTSTATUS LookupStatus = PsLookupProcessByProcessId(PsGetCurrentProcessId(), &RequesterProcess);
	if (NT_SUCCESS(LookupStatus))
	{
		uint8 Roles = 0;
		if (Context.GetAgentState().IsAgentProcess(RequesterProcess))
		{
			Roles |= static_cast<uint8>(Argus::Notify::ERegistryRequesterRole::Agent);
		}
		if (Context.GetProtectedProcessRegistry().IsProtectedProcess(RequesterProcess))
		{
			Roles |= static_cast<uint8>(Argus::Notify::ERegistryRequesterRole::Protected);
		}
		EIsolationFlags IsolationFlags = EIsolationFlags::None;
		if (Context.GetIsolationProcessRegistry().GetProcessFlags(RequesterProcess, IsolationFlags))
		{
			Roles |= static_cast<uint8>(Argus::Notify::ERegistryRequesterRole::Isolated);
			Operation.IsolationFlags = static_cast<uint32>(IsolationFlags);
		}
		Operation.RequesterRoles = static_cast<Argus::Notify::ERegistryRequesterRole>(Roles);
		ObDereferenceObject(RequesterProcess);
	}
	return Operation;
}

