#include "Kernel/Private/Notify/RegistryNotifyQueue.h"
#include "Kernel/Private/DriverContext.h"

static_assert(sizeof(ULONG_PTR) == sizeof(uint64),
	"Registry operation IDs must fit in CallContext");

namespace 
{
	// The configuration manager copies a pre-notification's CallContext to the
	// matching post-notification. Legacy XP create/open classes have no slot.
	PVOID* GetPreCallContext(REG_NOTIFY_CLASS NotifyClass, void* PreInformation) noexcept
	{
		if (PreInformation == nullptr)
		{
			return nullptr;
		}

		switch (NotifyClass)
		{
		case RegNtPreCreateKeyEx:
		case RegNtPreOpenKeyEx:
			return &static_cast<PREG_CREATE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreDeleteKey:
		case RegNtPreFlushKey:
			return &static_cast<PREG_DELETE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreSetValueKey:
			return &static_cast<PREG_SET_VALUE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreDeleteValueKey:
			return &static_cast<PREG_DELETE_VALUE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreSetInformationKey:
			return &static_cast<PREG_SET_INFORMATION_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreEnumerateKey:
			return &static_cast<PREG_ENUMERATE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreEnumerateValueKey:
			return &static_cast<PREG_ENUMERATE_VALUE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreQueryKey:
			return &static_cast<PREG_QUERY_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreQueryValueKey:
			return &static_cast<PREG_QUERY_VALUE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreQueryMultipleValueKey:
			return &static_cast<PREG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreRenameKey:
			return &static_cast<PREG_RENAME_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreKeyHandleClose:
			return &static_cast<PREG_KEY_HANDLE_CLOSE_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreLoadKey:
			return &static_cast<PREG_LOAD_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreUnLoadKey:
			return &static_cast<PREG_UNLOAD_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreQueryKeySecurity:
			return &static_cast<PREG_QUERY_KEY_SECURITY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreSetKeySecurity:
			return &static_cast<PREG_SET_KEY_SECURITY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreRestoreKey:
			return &static_cast<PREG_RESTORE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreSaveKey:
			return &static_cast<PREG_SAVE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreReplaceKey:
			return &static_cast<PREG_REPLACE_KEY_INFORMATION>(PreInformation)->CallContext;
		case RegNtPreQueryKeyName:
			return &static_cast<PREG_QUERY_KEY_NAME>(PreInformation)->CallContext;
		case RegNtPreSaveMergedKey:
			return &static_cast<PREG_SAVE_MERGED_KEY_INFORMATION>(PreInformation)->CallContext;
		default:
			return nullptr;
		}
	}

	void CaptureName(PCUNICODE_STRING Source, FString<>& Destination, Argus::Notify::ERegistryNameStatus& NameStatus) noexcept
	{
		if (Source == nullptr || (Source->Length % sizeof(WIDECHAR)) != 0 ||
			(Source->Length != 0 && Source->Buffer == nullptr))
		{
			Destination.Empty();
			NameStatus = Argus::Notify::ERegistryNameStatus::Unavailable;
			return;
		}

		// A zero-length value name is valid even when UNICODE_STRING::Buffer is null.
		const int32 CharacterCount = Source->Length / sizeof(WIDECHAR);
		const WIDECHAR* NameData = CharacterCount != 0 ? Source->Buffer : L"";
		if (!Destination.InitializeWithLength(NameData, CharacterCount))
		{
			Destination.Empty();
			NameStatus = Argus::Notify::ERegistryNameStatus::AllocationFailed;
			return;
		}
		NameStatus = Argus::Notify::ERegistryNameStatus::Captured;
	}

	void CapturePreNames(REG_NOTIFY_CLASS NotifyClass, void* PreInformation, Argus::Notify::FRegistryNotifyInfo& Info) noexcept
	{
		if (NotifyClass == RegNtPreSetValueKey || NotifyClass == RegNtPreDeleteValueKey ||
			NotifyClass == RegNtPreQueryValueKey)
		{
			Info.ValueNameStatus = Argus::Notify::ERegistryNameStatus::Unavailable;
		}

		if (PreInformation == nullptr)
		{
			return;
		}

		switch (NotifyClass)
		{
		case RegNtPreCreateKey:
			CaptureName(static_cast<PREG_PRE_CREATE_KEY_INFORMATION>(PreInformation)->CompleteName,
				Info.KeyName, Info.KeyNameStatus);
			break;
		case RegNtPreCreateKeyEx:
			CaptureName(static_cast<PREG_CREATE_KEY_INFORMATION>(PreInformation)->CompleteName,
				Info.KeyName, Info.KeyNameStatus);
			break;
		case RegNtPreOpenKey:
			CaptureName(static_cast<PREG_PRE_OPEN_KEY_INFORMATION>(PreInformation)->CompleteName,
				Info.KeyName, Info.KeyNameStatus);
			break;
		case RegNtPreOpenKeyEx:
			CaptureName(static_cast<PREG_OPEN_KEY_INFORMATION>(PreInformation)->CompleteName,
				Info.KeyName, Info.KeyNameStatus);
			break;
		case RegNtPreSetValueKey:
			CaptureName(static_cast<PREG_SET_VALUE_KEY_INFORMATION>(PreInformation)->ValueName,
				Info.ValueName, Info.ValueNameStatus);
			break;
		case RegNtPreDeleteValueKey:
			CaptureName(static_cast<PREG_DELETE_VALUE_KEY_INFORMATION>(PreInformation)->ValueName,
				Info.ValueName, Info.ValueNameStatus);
			break;
		case RegNtPreQueryValueKey:
			CaptureName(static_cast<PREG_QUERY_VALUE_KEY_INFORMATION>(PreInformation)->ValueName,
				Info.ValueName, Info.ValueNameStatus);
			break;
		default:
			// Object-based operations need a separate, validated key-name lookup.
			break;
		}
	}

	REG_NOTIFY_CLASS GetPreNotifyClass(REG_NOTIFY_CLASS PostClass) noexcept
	{
		switch (PostClass)
		{
		case RegNtPostCreateKeyEx: return RegNtPreCreateKeyEx;
		case RegNtPostOpenKeyEx: return RegNtPreOpenKeyEx;
		case RegNtPostSetValueKey: return RegNtPreSetValueKey;
		case RegNtPostDeleteValueKey: return RegNtPreDeleteValueKey;
		case RegNtPostQueryValueKey: return RegNtPreQueryValueKey;
		default: return MaxRegNtNotifyClass;
		}
	}
}

void FRegistryNotifyQueue::OnRegistryNotify(NTSTATUS CallbackStatus, void* Context, void* ArgumentA, void* ArgumentB) noexcept
{
	auto* DriverContext = static_cast<FDriverContext*>(Context);
	if (DriverContext == nullptr)
	{
		return;
	}

	const FRegistryOperationContext Operation = BuildRegistryOperationContext(
		*DriverContext, ArgumentA, ArgumentB);
	switch (Operation.Phase)
	{
	case Argus::Notify::ERegistryNotifyPhase::Pre:
		this->OnPreRegistryNotify(Operation, CallbackStatus);
		break;
	case Argus::Notify::ERegistryNotifyPhase::Post:
		this->OnPostRegistryNotify(Operation);
		break;
	case Argus::Notify::ERegistryNotifyPhase::Unknown:
	default:
		break;
	}
}

void FRegistryNotifyQueue::OnPreRegistryNotify(const FRegistryOperationContext& Operation, NTSTATUS DecisionStatus) noexcept
{
	if (Operation.RequesterRoles == Argus::Notify::ERegistryRequesterRole::None ||
		Operation.OperationType == Argus::Notify::ERegistryOperationType::Unknown)
	{
		return;
	}

	uint64 OperationId = 0;
	if (PVOID* CallContext = GetPreCallContext(Operation.NotifyClass, Operation.Information))
	{
		// Zero is reserved for legacy notifications without CallContext.
		do
		{
			OperationId = this->NextOperationId.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
		} while (OperationId == 0);
		*CallContext = reinterpret_cast<PVOID>(static_cast<ULONG_PTR>(OperationId));
	}

	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FRegistryNotifyInfo Info{};
	KeQuerySystemTimePrecise(&Info.OccurredAt);
	Info.OperationId = OperationId;
	Info.OperationType = Operation.OperationType;
	Info.Phase = Argus::Notify::ERegistryNotifyPhase::Pre;
	Info.RequesterRoles = Operation.RequesterRoles;
	Info.ProcessId = Operation.ProcessId;
	Info.ThreadId = Operation.ThreadId;
	Info.IsolationFlags = Operation.IsolationFlags;
	Info.Status = DecisionStatus;
	Info.Decision = NT_SUCCESS(DecisionStatus)
		? Argus::Notify::ERegistryDecision::Allowed
		: Argus::Notify::ERegistryDecision::Blocked;
	CapturePreNames(Operation.NotifyClass, Operation.Information, Info);
	this->EnqueueReady(MoveTemp(Info));
}

void FRegistryNotifyQueue::OnPostRegistryNotify(const FRegistryOperationContext& Operation) noexcept
{
	if (Operation.RequesterRoles == Argus::Notify::ERegistryRequesterRole::None ||
		Operation.OperationType == Argus::Notify::ERegistryOperationType::Unknown ||
		Operation.Information == nullptr || !this->TryRestore())
	{
		return;
	}

	Argus::Notify::FRegistryNotifyInfo Info{};
	KeQuerySystemTimePrecise(&Info.OccurredAt);
	Info.OperationType = Operation.OperationType;
	Info.Phase = Argus::Notify::ERegistryNotifyPhase::Post;
	Info.RequesterRoles = Operation.RequesterRoles;
	Info.ProcessId = Operation.ProcessId;
	Info.ThreadId = Operation.ThreadId;
	Info.IsolationFlags = Operation.IsolationFlags;
	Info.Decision = Argus::Notify::ERegistryDecision::Allowed;

	if (Operation.NotifyClass == RegNtPostCreateKey || Operation.NotifyClass == RegNtPostOpenKey)
	{
		// Legacy create/open notifications use a different post structure.
		auto* Post = static_cast<PREG_POST_CREATE_KEY_INFORMATION>(Operation.Information);
		Info.Status = Post->Status;
		CaptureName(Post->CompleteName, Info.KeyName, Info.KeyNameStatus);
	}
	else
	{
		auto* Post = static_cast<PREG_POST_OPERATION_INFORMATION>(Operation.Information);
		Info.OperationId = static_cast<uint64>(reinterpret_cast<ULONG_PTR>(Post->CallContext));
		Info.Status = Post->Status;
		CapturePreNames(GetPreNotifyClass(Operation.NotifyClass), Post->PreInformation, Info);
	}
	this->EnqueueReady(MoveTemp(Info));
}

void FRegistryNotifyQueue::EnqueueReady(Argus::Notify::FRegistryNotifyInfo&& Info) noexcept
{
	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->RegistryNotifyInfosLock);

	Argus::Notify::FRegistryNotifyInfo EvictedInfo{};
	const bool bWasFull = this->RegistryNotifyInfos.IsFull();
	const bool bPushSucceed =
		this->RegistryNotifyInfos.PushDropOldestIfFull(MoveTemp(Info), &EvictedInfo) != INDEX_NONE;
	if (!bPushSucceed || bWasFull)
	{
		++this->DroppedRegistryNotifyCount;
	}

	ExReleasePushLockExclusive(&this->RegistryNotifyInfosLock);
	KeLeaveCriticalRegion();
}
