#include "Kernel/Private/Callback/ProcessCallback.h"
#include "Kernel/Private/DriverContext.h"
#include "Kernel/Private/Logging.h"

extern "C" UCHAR* PsGetProcessImageFileName(PEPROCESS Process);

void FProcessCallback::OnProcessCreateNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	CORE_CHECK(GDriverContext != nullptr);
	this->ApplyRule(*GDriverContext, Process, ProcessId, CreateInfo);
}

void FProcessCallback::OnProcessExitNotify(PEPROCESS Process, HANDLE ProcessId)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);
}

void FProcessCallback::ApplyRule(FDriverContext& Context, PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) noexcept
{
	FProcessCreateRules::ApplyRule(Context, Process, ProcessId, CreateInfo);
}

void FProcessCallback::FProcessCreateRules::ApplyRule(FDriverContext& Context, PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) noexcept
{
	UNREFERENCED_PARAMETER(Process);

	PEPROCESS CreatorProcess = nullptr;
	const NTSTATUS LookupStatus = PsLookupProcessByProcessId(CreateInfo->CreatingThreadId.UniqueProcess, &CreatorProcess);
	if (!NT_SUCCESS(LookupStatus))
	{
		return;
	}

	auto& IsoProcessRegistry = Context.GetIsolationProcessRegistry();
	const bool bIsIsolationProcess = IsoProcessRegistry.IsIsolatedProcess(CreatorProcess);
	if (bIsIsolationProcess)
	{
		Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "AUDIT", "process.create_detect",
			"role=isolation creator_image=\"%.15s\" creator_pid=%lu child_pid=%lu\n",
			reinterpret_cast<const char*>(PsGetProcessImageFileName(CreatorProcess)),
			HandleToULong(PsGetProcessId(CreatorProcess)),
			HandleToULong(ProcessId)
		);

		ApplyIsolationProcessRule(Context, CreatorProcess, ProcessId, CreateInfo);
	}

	ObDereferenceObject(CreatorProcess);
}

void FProcessCallback::FProcessCreateRules::ApplyIsolationProcessRule(FDriverContext& Context, PEPROCESS CreatorProcess, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) noexcept
{
	UNREFERENCED_PARAMETER(ProcessId);

	EIsolationFlags Flags = EIsolationFlags::None;
	const bool bGetFlagsSucceed = Context.GetIsolationProcessRegistry().GetProcessFlags(CreatorProcess, Flags);
	if (bGetFlagsSucceed)
	{
		const uint32 CurrentFlags = static_cast<uint32>(Flags);
		const uint32 TargetFlags  = static_cast<uint32>(EIsolationFlags::DenyChildProcessCreation);
		const bool bIsDenyIsoProcess = (CurrentFlags & TargetFlags) != 0;
		if (bIsDenyIsoProcess)
		{
			CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;
		}
	}
}

void FProcessCallback::FProcessCreateRules::ApplyProtectedProcessRule(FDriverContext& Context, PEPROCESS CreatorProcess, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) noexcept
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(CreatorProcess);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(CreateInfo);
}
