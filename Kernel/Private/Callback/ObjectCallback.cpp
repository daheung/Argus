#include "ObjectCallback.h"
#include "Kernel/Private/DriverContext.h"
#include "Kernel/Private/Logging.h"

extern "C" UCHAR* PsGetProcessImageFileName(PEPROCESS Process);


namespace
{
	/**
	 * @brief Gets the requested-access field that a pre-callback can reduce.
	 * @param Information The pending handle create or duplicate operation.
	 * @return The requested-access field, or nullptr for an unsupported operation.
	 */
	ACCESS_MASK* GetDesiredAccess(OB_PRE_OPERATION_INFORMATION& Information) noexcept
	{
		switch (Information.Operation)
		{
		case OB_OPERATION_HANDLE_CREATE:
			return &Information.Parameters->CreateHandleInformation.DesiredAccess;
		case OB_OPERATION_HANDLE_DUPLICATE:
			return &Information.Parameters->DuplicateHandleInformation.DesiredAccess;
		default:
			return nullptr;
		}
	}

	constexpr ACCESS_MASK ProcessTerminate		 = 0x0001;
	constexpr ACCESS_MASK ProcessCreateThread	 = 0x0002;
	constexpr ACCESS_MASK ProcessVMOperation	 = 0x0008;
	constexpr ACCESS_MASK ProcessVMWrite		 = 0x0020;
	constexpr ACCESS_MASK ProcessDupHandle		 = 0x0040;
	constexpr ACCESS_MASK ProcessCreateProcess	 = 0x0080;
	constexpr ACCESS_MASK ProcessSetQuota		 = 0x0100;
	constexpr ACCESS_MASK ProcessSetInformation  = 0x0200;
	constexpr ACCESS_MASK ProcessSuspendResume	 = 0x0800;

	constexpr ACCESS_MASK ThreadTerminate			  = 0x0001;
	constexpr ACCESS_MASK ThreadSuspendResume		  = 0x0002;
	constexpr ACCESS_MASK ThreadSetContext			  = 0x0010;
	constexpr ACCESS_MASK ThreadSetInformation		  = 0x0020;
	constexpr ACCESS_MASK ThreadSetThreadToken		  = 0x0080;
	constexpr ACCESS_MASK ThreadImpersonate			  = 0x0100;
	constexpr ACCESS_MASK ThreadDirectImpersonation	  = 0x0200;
	constexpr ACCESS_MASK ThreadSetLimitedInformation = 0x0400;

	UCHAR TerminateAccessRemovedContext = 0;
}

PVOID FObjectCallback::GetTerminateAccessRemovedContext() noexcept
{
	return &TerminateAccessRemovedContext;
}

PEPROCESS FObjectCallback::GetHandleOwnerProcess(POB_PRE_OPERATION_INFORMATION Information) noexcept
{
	if (Information == nullptr || Information->Parameters == nullptr)
	{
		return nullptr;
	}

	switch (Information->Operation)
	{
	case OB_OPERATION_HANDLE_CREATE:
		return PsGetCurrentProcess();
	case OB_OPERATION_HANDLE_DUPLICATE:
		return static_cast<PEPROCESS>(Information->Parameters->DuplicateHandleInformation.TargetProcess);
	default:
		return nullptr;
	}
}

OB_PREOP_CALLBACK_STATUS FObjectCallback::OnPreOperationCallback(void* RegistrationContext, POB_PRE_OPERATION_INFORMATION Information) noexcept
{
	auto* Context = static_cast<FDriverContext*>(RegistrationContext);
	CORE_CHECK(Context != nullptr);

	if (Information == nullptr || Information->KernelHandle || Information->Parameters == nullptr)
	{
		return OB_PREOP_SUCCESS;
	}
	if (Information->Operation != OB_OPERATION_HANDLE_CREATE &&
		Information->Operation != OB_OPERATION_HANDLE_DUPLICATE)
	{
		return OB_PREOP_SUCCESS;
	}

	this->ApplyPreOperationRules(*Context, Information);

	return OB_PREOP_SUCCESS;
}

void FObjectCallback::OnPostOperationCallback(void* RegistrationContext, POB_POST_OPERATION_INFORMATION Information) noexcept
{
	auto* Context = static_cast<FDriverContext*>(RegistrationContext);
	CORE_CHECK(Context != nullptr);

	if (Information == nullptr || Information->KernelHandle ||
		Information->ObjectType != *PsProcessType || Information->Parameters == nullptr ||
		(Information->CallContext != &TerminateAccessRemovedContext &&
			!Context->GetObjectOperationNotifyQueue().HasPreviousCallContext(
				Information->CallContext, &TerminateAccessRemovedContext)) ||
		!NT_SUCCESS(Information->ReturnStatus))
	{
		return;
	}

	ACCESS_MASK GrantedAccess = 0;
	const char* OperationName = nullptr;
	switch (Information->Operation)
	{
	case OB_OPERATION_HANDLE_CREATE:
		GrantedAccess = Information->Parameters->CreateHandleInformation.GrantedAccess;
		OperationName = "CREATE";
		break;
	case OB_OPERATION_HANDLE_DUPLICATE:
		GrantedAccess = Information->Parameters->DuplicateHandleInformation.GrantedAccess;
		OperationName = "DUPLICATE";
		break;
	default:
		return;
	}

	if ((GrantedAccess & ProcessTerminate) != 0)
	{
		PEPROCESS TargetProcess = static_cast<PEPROCESS>(Information->Object);
		Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN", "handle.rights_not_removed",
			"operation=%s target_image=\"%.15s\" target_pid=%lu granted=0x%X\n",
			OperationName,
			reinterpret_cast<const char*>(PsGetProcessImageFileName(TargetProcess)),
			HandleToULong(PsGetProcessId(TargetProcess)),
			GrantedAccess
		);
	}
}

void FObjectCallback::ApplyPreOperationRules(FDriverContext& Context, POB_PRE_OPERATION_INFORMATION Information) noexcept
{
	PEPROCESS HandleOwnerProcess = GetHandleOwnerProcess(Information);
	if (HandleOwnerProcess == nullptr)
	{
		return;
	}
	
	if (Information->ObjectType == *PsProcessType)
	{
		PEPROCESS TargetProcess = static_cast<PEPROCESS>(Information->Object);
		FPreOperationProcessRules::ApplyRule(Context, HandleOwnerProcess, TargetProcess, *Information);
	}
	else if (Information->ObjectType == *PsThreadType)
	{
		PETHREAD TargetThread = static_cast<PETHREAD>(Information->Object);
		FPreOperationThreadRules::ApplyRule(Context, HandleOwnerProcess, TargetThread, *Information);
	}
}

void FObjectCallback::FPreOperationProcessRules::ApplyRule(
	FDriverContext& Context, 
	PEPROCESS HandleOwnerProcess, 
	PEPROCESS TargetProcess, 
	OB_PRE_OPERATION_INFORMATION& Information
) noexcept 
{
	auto& AgentState = Context.GetAgentState();
	const bool bIsAgentProcess = AgentState.IsAgentProcess(TargetProcess);

	auto& ProtProcessRegistry = Context.GetProtectedProcessRegistry();
	const bool bIsProtectedProcess = ProtProcessRegistry.IsProtectedProcess(TargetProcess);

	auto& IsoProcessRegistry = Context.GetIsolationProcessRegistry();
	const bool bIsIsolationProcess = IsoProcessRegistry.IsIsolatedProcess(HandleOwnerProcess);

	// Agent and protected status describe the target process; isolation status describes the handle owner.
	// For a duplicated handle, the owner is the process receiving that handle.
	// Skip requests with no matching policy and handles owned by their target process.
	const bool bIsTargetProcess = bIsAgentProcess || bIsProtectedProcess || bIsIsolationProcess;
	if (bIsTargetProcess)
	{
		const char* OperationName = Information.Operation == OB_OPERATION_HANDLE_CREATE ? "CREATE" : "DUPLICATE";
		Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "AUDIT", "handle.detect",
			"operation=%s owner_image=\"%.15s\" owner_pid=%lu target_image=\"%.15s\" target_pid=%lu "
			"agent_target=%u protected_target=%u isolated_owner=%u\n",
			OperationName,
			reinterpret_cast<const char*>(PsGetProcessImageFileName(HandleOwnerProcess)),
			HandleToULong(PsGetProcessId(HandleOwnerProcess)),
			reinterpret_cast<const char*>(PsGetProcessImageFileName(TargetProcess)),
			HandleToULong(PsGetProcessId(TargetProcess)),
			bIsAgentProcess ? 1u : 0u,
			bIsProtectedProcess ? 1u : 0u,
			bIsIsolationProcess ? 1u : 0u
		);
	}
	const bool bIsTargetItself = HandleOwnerProcess == TargetProcess;
	if (!bIsTargetProcess || bIsTargetItself)
	{
		return;
	}

	ACCESS_MASK* DesiredAccess = GetDesiredAccess(Information);
	if (DesiredAccess == nullptr)
	{
		return;
	}

	const ACCESS_MASK BeforeMask = *DesiredAccess;

	if (bIsAgentProcess)
	{
		ApplyAgentProcessRule(Context, HandleOwnerProcess, TargetProcess, Information);
	}
	if (bIsProtectedProcess)
	{
		ApplyProtectedProcessRule(Context, HandleOwnerProcess, TargetProcess, Information);
	}
	if (bIsIsolationProcess)
	{
		ApplyIsolationProcessRule(Context, HandleOwnerProcess, TargetProcess, Information);
	}
	if ((BeforeMask & ProcessTerminate) != 0 &&
		(*DesiredAccess & ProcessTerminate) == 0)
	{
		Information.CallContext = &TerminateAccessRemovedContext;
	}
}

void FObjectCallback::FPreOperationProcessRules::ApplyAgentProcessRule(
	FDriverContext& Context, 
	PEPROCESS HandleOwnerProcess, 
	PEPROCESS TargetProcess, 
	OB_PRE_OPERATION_INFORMATION& Information
) noexcept
{
	UNREFERENCED_PARAMETER(Context);
	if (HandleOwnerProcess == TargetProcess)
	{
		return;
	}

	ACCESS_MASK* DesiredAccess = GetDesiredAccess(Information);
	if (DesiredAccess != nullptr)
	{
		const ACCESS_MASK BeforeMask = *DesiredAccess;
		*DesiredAccess &= ~ProcessTerminate;
		const ACCESS_MASK RemovedMask = BeforeMask & ~*DesiredAccess;
		if (RemovedMask != 0)
		{
			const char* OperationName = Information.Operation == OB_OPERATION_HANDLE_CREATE ? "CREATE" : "DUPLICATE";
			Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN", "handle.rights_reduced",
				"role=agent operation=%s owner_image=\"%.15s\" owner_pid=%lu target_image=\"%.15s\" target_pid=%lu "
				"removed=0x%X access_before=0x%X access_after=0x%X\n",
				OperationName,
				reinterpret_cast<const char*>(PsGetProcessImageFileName(HandleOwnerProcess)),
				HandleToULong(PsGetProcessId(HandleOwnerProcess)),
				reinterpret_cast<const char*>(PsGetProcessImageFileName(TargetProcess)),
				HandleToULong(PsGetProcessId(TargetProcess)),
				RemovedMask,
				BeforeMask,
				*DesiredAccess
			);
		}
	}
}

void FObjectCallback::FPreOperationProcessRules::ApplyProtectedProcessRule(
	FDriverContext& Context, 
	PEPROCESS HandleOwnerProcess, 
	PEPROCESS TargetProcess, 
	OB_PRE_OPERATION_INFORMATION& Information
) noexcept 
{
	UNREFERENCED_PARAMETER(Context);
	if (HandleOwnerProcess == TargetProcess)
	{
		return;
	}

	ACCESS_MASK* DesiredAccess = GetDesiredAccess(Information);
	if (DesiredAccess != nullptr)
	{
		constexpr ACCESS_MASK RemoveMask = 
			ProcessTerminate	|	// Terminate the target process.
			ProcessCreateThread		|	// Create a thread in the target process.
			ProcessVMOperation		|	// Change the target process's address space.
			ProcessVMWrite			|	// Write to the target process's memory.
			ProcessDupHandle		|	// Duplicate handles to or from the target process.
			ProcessCreateProcess;		// Use the target process as a new process's parent.

		const ACCESS_MASK BeforeMask = *DesiredAccess;
		*DesiredAccess &= ~RemoveMask;
		const ACCESS_MASK RemovedMask = BeforeMask & ~*DesiredAccess;
		if (RemovedMask != 0)
		{
			const char* OperationName = Information.Operation == OB_OPERATION_HANDLE_CREATE ? "CREATE" : "DUPLICATE";
			Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN", "handle.rights_reduced",
				"role=protected operation=%s owner_image=\"%.15s\" owner_pid=%lu target_image=\"%.15s\" target_pid=%lu "
				"removed=0x%X access_before=0x%X access_after=0x%X\n",
				OperationName,
				reinterpret_cast<const char*>(PsGetProcessImageFileName(HandleOwnerProcess)),
				HandleToULong(PsGetProcessId(HandleOwnerProcess)),
				reinterpret_cast<const char*>(PsGetProcessImageFileName(TargetProcess)),
				HandleToULong(PsGetProcessId(TargetProcess)),
				RemovedMask,
				BeforeMask,
				*DesiredAccess
			);
		}
	}
}

void FObjectCallback::FPreOperationProcessRules::ApplyIsolationProcessRule(
	FDriverContext& Context, 
	PEPROCESS HandleOwnerProcess, 
	PEPROCESS TargetProcess, 
	OB_PRE_OPERATION_INFORMATION& Information
) noexcept
{
	if (HandleOwnerProcess == TargetProcess)
	{
		return;
	}

	ACCESS_MASK* DesiredAccess = GetDesiredAccess(Information);
	if (DesiredAccess != nullptr)
	{
		EIsolationFlags Flags = EIsolationFlags::None;
		const bool bGetFlagsSucceed = Context.GetIsolationProcessRegistry().GetProcessFlags(HandleOwnerProcess, Flags);
		if (!bGetFlagsSucceed)
		{
			return;
		}

		const uint32 CurrentFlags = static_cast<uint32>(Flags);
		const uint32 TargetFlags  = static_cast<uint32>(EIsolationFlags::RestrictProcessHandleRights);
		const bool bIsRestrictIsoProcess = (CurrentFlags & TargetFlags) != 0;
		if (!bIsRestrictIsoProcess)
		{
			return;
		}

		constexpr ACCESS_MASK RemoveMask = 
			ProcessTerminate		|	// Terminate the target process.
			ProcessCreateThread		|	// Create a thread in the target process.
			ProcessVMOperation		|	// Change the target process's address space.
			ProcessVMWrite			|	// Write to the target process's memory.
			ProcessDupHandle		|	// Duplicate handles to or from the target process.
			ProcessCreateProcess	|	// Use the target process as a new process's parent.
			ProcessSuspendResume;		// Suspend or resume the target process.

		const ACCESS_MASK BeforeMask = *DesiredAccess;
		*DesiredAccess &= ~RemoveMask;
		const ACCESS_MASK RemovedMask = BeforeMask & ~*DesiredAccess;
		if (RemovedMask != 0)
		{
			const char* OperationName = Information.Operation == OB_OPERATION_HANDLE_CREATE ? "CREATE" : "DUPLICATE";
			Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN", "handle.rights_reduced",
				"role=isolation operation=%s owner_image=\"%.15s\" owner_pid=%lu target_image=\"%.15s\" target_pid=%lu "
				"removed=0x%X access_before=0x%X access_after=0x%X\n",
				OperationName,
				reinterpret_cast<const char*>(PsGetProcessImageFileName(HandleOwnerProcess)),
				HandleToULong(PsGetProcessId(HandleOwnerProcess)),
				reinterpret_cast<const char*>(PsGetProcessImageFileName(TargetProcess)),
				HandleToULong(PsGetProcessId(TargetProcess)),
				RemovedMask,
				BeforeMask,
				*DesiredAccess
			);
		}
	}
}

void FObjectCallback::FPreOperationThreadRules::ApplyRule(
	FDriverContext& Context, 
	PEPROCESS HandleOwnerProcess, 
	PETHREAD TargetThread, 
	OB_PRE_OPERATION_INFORMATION& Information
) noexcept
{
	PEPROCESS TargetProcess = IoThreadToProcess(TargetThread);
	if (TargetProcess == nullptr)
	{
		return;
	}

	auto& AgentState = Context.GetAgentState();
	const bool bIsAgentProcess = AgentState.IsAgentProcess(TargetProcess);

	auto& ProtProcessRegistry = Context.GetProtectedProcessRegistry();
	const bool bIsProtectedProcess = ProtProcessRegistry.IsProtectedProcess(TargetProcess);

	auto& IsoProcessRegistry = Context.GetIsolationProcessRegistry();
	const bool bIsIsolationProcess = IsoProcessRegistry.IsIsolatedProcess(HandleOwnerProcess);

	// Agent and protected status describe the target process; isolation status describes the handle owner.
	// For a duplicated handle, the owner is the process receiving that handle.
	// Skip requests with no matching policy and handles owned by their target process.
	const bool bIsTargetProcess = bIsAgentProcess || bIsProtectedProcess || bIsIsolationProcess;
	if (bIsTargetProcess)
	{
		const char* OperationName = Information.Operation == OB_OPERATION_HANDLE_CREATE ? "CREATE" : "DUPLICATE";
		Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "AUDIT", "handle.detect",
			"operation=%s owner_image=\"%.15s\" owner_pid=%lu target_image=\"%.15s\" target_pid=%lu "
			"agent_target=%u protected_target=%u isolated_owner=%u\n",
			OperationName,
			reinterpret_cast<const char*>(PsGetProcessImageFileName(HandleOwnerProcess)),
			HandleToULong(PsGetProcessId(HandleOwnerProcess)),
			reinterpret_cast<const char*>(PsGetProcessImageFileName(TargetProcess)),
			HandleToULong(PsGetProcessId(TargetProcess)),
			bIsAgentProcess ? 1u : 0u,
			bIsProtectedProcess ? 1u : 0u,
			bIsIsolationProcess ? 1u : 0u
		);
	}
	const bool bIsTargetItself = HandleOwnerProcess == TargetProcess;
	if (!bIsTargetProcess || bIsTargetItself)
	{
		return;
	}

	ACCESS_MASK* DesiredAccess = GetDesiredAccess(Information);
	if (DesiredAccess == nullptr)
	{
		return;
	}

	const ACCESS_MASK BeforeMask = *DesiredAccess;

	if (bIsAgentProcess)
	{
		// Is Agent's thread
	}
	if (bIsProtectedProcess)
	{
		ApplyProtectedThreadRule(Context, HandleOwnerProcess, TargetProcess, TargetThread, Information);
	}
	if (bIsIsolationProcess)
	{
	}
	if ((BeforeMask & ProcessTerminate) != 0 &&
		(*DesiredAccess & ProcessTerminate) == 0)
	{
		Information.CallContext = &TerminateAccessRemovedContext;
	}
}

void FObjectCallback::FPreOperationThreadRules::ApplyProtectedThreadRule(
	FDriverContext& Context, 
	PEPROCESS HandleOwnerProcess, 
	PEPROCESS TargetProcess,
	PETHREAD TargetThread, 
	OB_PRE_OPERATION_INFORMATION& Information
) noexcept
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(TargetThread);

	if (HandleOwnerProcess == TargetProcess)
	{
		return;
	}

	ACCESS_MASK* DesiredAccess = GetDesiredAccess(Information);
	if (DesiredAccess != nullptr)
	{
		constexpr ACCESS_MASK RemoveMask =
			ThreadTerminate				|	// Terminate the target thread.
			ThreadSuspendResume			|	// Suspend or resume the target thread.
			ThreadSetContext			|	// Change the target thread's execution context.
			ThreadSetInformation		|	// Change the target thread's information.
			ThreadSetThreadToken		|	// Set the target thread's impersonation token.
			ThreadImpersonate			|	// Use the target thread's security context for impersonation.
			ThreadDirectImpersonation	|	// Allow the target server thread to impersonate a client directly.
			ThreadSetLimitedInformation;	// Change limited information about the target thread.

		const ACCESS_MASK BeforeMask = *DesiredAccess;
		*DesiredAccess &= ~RemoveMask;
		const ACCESS_MASK RemovedMask = BeforeMask & ~*DesiredAccess;
		if (RemovedMask != 0)
		{
			const char* OperationName = Information.Operation == OB_OPERATION_HANDLE_CREATE ? "CREATE" : "DUPLICATE";
			Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN", "handle.rights_reduced",
				"role=protected operation=%s owner_image=\"%.15s\" owner_pid=%lu target_image=\"%.15s\" target_pid=%lu "
				"removed=0x%X access_before=0x%X access_after=0x%X\n",
				OperationName,
				reinterpret_cast<const char*>(PsGetProcessImageFileName(HandleOwnerProcess)),
				HandleToULong(PsGetProcessId(HandleOwnerProcess)),
				reinterpret_cast<const char*>(PsGetProcessImageFileName(TargetProcess)),
				HandleToULong(PsGetProcessId(TargetProcess)),
				RemovedMask,
				BeforeMask,
				*DesiredAccess
			);
		}
	}
}
