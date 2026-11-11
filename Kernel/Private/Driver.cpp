#include "Core/Public/Security/ArgusIdentity.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Private/DriverContext.h"
#include "Kernel/Private/DeviceControl/DeviceControl.h"
#include "Kernel/Private/Logging.h"

FDriverContext* GDriverContext = nullptr;

static void HandleProcessNotifyRoutineFunc(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	const bool bIsCreateNotify = CreateInfo != nullptr;
	if (bIsCreateNotify)
	{
		Argus::Notify::FProcessNotifyPolicy ProcessPolicy;

		// Inspect the creation request and apply policy changes first.
		// FProcessCallback can reject creation through CreateInfo->CreationStatus.
		GDriverContext->GetProcessCallback().OnProcessCreateNotify(Process, ProcessId, CreateInfo, ProcessPolicy);
	
		// Forward the request to the Agent even if creation was rejected.
		GDriverContext->GetProcessNotifyQueue().OnProcessCreateNotify(Process, ProcessId, CreateInfo, ProcessPolicy);
	}
	else
	{
		// Process exit cannot be rejected; use this callback for policy cleanup.
		GDriverContext->GetProcessCallback().OnProcessExitNotify(Process, ProcessId);

		// Release process references retained by the two independent registries.
		GDriverContext->GetProtectedProcessRegistry().UnregisterProcess(Process, nullptr);
		GDriverContext->GetIsolationProcessRegistry().UnregisterProcess(Process, nullptr);

		// Clear the Agent registration if the exiting process is the Agent.
		GDriverContext->GetAgentState().OnProcessExit(Process);

		// Forward the exit notification to the user-mode Agent.
		GDriverContext->GetProcessNotifyQueue().OnProcessExitNotify(Process, ProcessId);
	}
}

static void HandleThreadNotifyRoutineFunc(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN bCreate)
{
	const bool bIsCreateNotify = bCreate;
	if (bIsCreateNotify)
	{
		GDriverContext->GetThreadNotifyQueue().OnThreadCreateNotify(ProcessId, ThreadId);
	}
	else
	{
		GDriverContext->GetThreadNotifyQueue().OnThreadExitNotify(ProcessId, ThreadId);
	}
}

static void HandleImageNotifyRoutineFunc(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	GDriverContext->GetImageNotifyQueue().OnImageNotify(FullImageName, ProcessId, ImageInfo);
}

static OB_PREOP_CALLBACK_STATUS HandlePreObjectOperationFunc(void* RegistrationContext, POB_PRE_OPERATION_INFORMATION Info)
{
	auto* Context = static_cast<FDriverContext*>(RegistrationContext);
	ACCESS_MASK* DesiredAccess = nullptr;
	if (Info != nullptr && !Info->KernelHandle && Info->Parameters != nullptr)
	{
		switch (Info->Operation)
		{
		case OB_OPERATION_HANDLE_CREATE:
			DesiredAccess = &Info->Parameters->CreateHandleInformation.DesiredAccess;
			break;
		case OB_OPERATION_HANDLE_DUPLICATE:
			DesiredAccess = &Info->Parameters->DuplicateHandleInformation.DesiredAccess;
			break;
		default:
			break;
		}
	}
	const ACCESS_MASK DesiredAccessBeforePolicy = DesiredAccess != nullptr ? *DesiredAccess : 0;

	OB_PREOP_CALLBACK_STATUS Status = Context->GetObjectCallback().OnPreOperationCallback(RegistrationContext, Info);

	if (DesiredAccess != nullptr)
	{
		Context->GetObjectOperationNotifyQueue().OnPreOperation(Info, DesiredAccessBeforePolicy);
	}

	return Status;
}

static void HandlePostObjectOperationFunc(void* RegistrationContext, POB_POST_OPERATION_INFORMATION Info)
{
	auto* Context = static_cast<FDriverContext*>(RegistrationContext);

	Context->GetObjectCallback().OnPostOperationCallback(RegistrationContext, Info);

	Context->GetObjectOperationNotifyQueue().OnPostOperation(Info);
}


static NTSTATUS HandleRegistryCallbackFunc(void* Context, void* ArgumentA, void* ArgumentB)
{
	auto* DriverContext = static_cast<FDriverContext*>(Context);
	if (DriverContext == nullptr)
	{
		return STATUS_SUCCESS;
	}

	const NTSTATUS CallbackStatus = DriverContext->GetRegistryCallback().HandleRegistryCallback(*DriverContext, ArgumentA, ArgumentB);

	DriverContext->GetRegistryNotifyQueue().OnRegistryNotify(CallbackStatus, Context, ArgumentA, ArgumentB);
	
	return CallbackStatus;
}

NTSTATUS FDriverContext::InitializeCallbacks(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) noexcept
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS Status = STATUS_SUCCESS;
	
	Status = PsSetCreateProcessNotifyRoutineEx(HandleProcessNotifyRoutineFunc, false);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	this->bProcessNotifyRegistered = true;

	Status = PsSetCreateThreadNotifyRoutine(HandleThreadNotifyRoutineFunc);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	this->bThreadNotifyRegistered = true;

	Status = PsSetLoadImageNotifyRoutine(HandleImageNotifyRoutineFunc);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	this->bImageNotifyRegistered = true;

	OB_OPERATION_REGISTRATION Operations[] = {
		{
			PsProcessType,
			OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
			HandlePreObjectOperationFunc, HandlePostObjectOperationFunc
		},
		{
			PsThreadType,
			OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
			HandlePreObjectOperationFunc, HandlePostObjectOperationFunc
		}
	};
	OB_CALLBACK_REGISTRATION Registration = {
		OB_FLT_REGISTRATION_VERSION,
		static_cast<USHORT>(RTL_NUMBER_OF(Operations)),
		RTL_CONSTANT_STRING(L"12345.12345"),
		this,
		Operations
	};
	PVOID RegistrationHandle = nullptr;
	Status = ObRegisterCallbacks(&Registration, &RegistrationHandle);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	this->ObCallbackRegistrationHandle = RegistrationHandle;

	LARGE_INTEGER RegistryCookie{};
	UNICODE_STRING RegistryAltitude = RTL_CONSTANT_STRING(L"7657.124");
	Status = CmRegisterCallbackEx(HandleRegistryCallbackFunc, &RegistryAltitude, DriverObject, this, &RegistryCookie, nullptr);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	this->CmCallbackRegistryCookie = RegistryCookie;
	this->bRegistryCallbackRegistered = true;

	return STATUS_SUCCESS;
}

void FDriverContext::ShutdownCallbacks() noexcept
{
	if (this->bRegistryCallbackRegistered)
	{
		CmUnRegisterCallback(this->CmCallbackRegistryCookie);
		this->CmCallbackRegistryCookie = {};
		this->bRegistryCallbackRegistered = false;
	}

	if (this->ObCallbackRegistrationHandle != nullptr)
	{
		ObUnRegisterCallbacks(this->ObCallbackRegistrationHandle);
		this->ObCallbackRegistrationHandle = nullptr;
	}

	if (this->bImageNotifyRegistered)
	{
		NTSTATUS Status = PsRemoveLoadImageNotifyRoutine(HandleImageNotifyRoutineFunc);
		CORE_CHECK(NT_SUCCESS(Status));
		this->bImageNotifyRegistered = false;
	}

	if (this->bThreadNotifyRegistered)
	{
		NTSTATUS Status = PsRemoveCreateThreadNotifyRoutine(HandleThreadNotifyRoutineFunc);
		CORE_CHECK(NT_SUCCESS(Status));
		this->bThreadNotifyRegistered = false;
	}

	if (this->bProcessNotifyRegistered)
	{
		NTSTATUS Status = PsSetCreateProcessNotifyRoutineEx(HandleProcessNotifyRoutineFunc, true);
		CORE_CHECK(NT_SUCCESS(Status));
		this->bProcessNotifyRegistered = false;
	}
}

static void CleanupDriver() noexcept
{
	if (GDriverContext == nullptr)
	{
		return;
	}

	// Unregister file I/O callbacks before freeing their owning context.
	GDriverContext->GetMinifilter().UnregisterFilter();

	if (GDriverContext->IsSymbolicLinkCreated())
	{
		UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\??\\Argus");
		IoDeleteSymbolicLink(&SymlinkName);
		GDriverContext->SetSymbolicLinkCreated(false);
	}

	GDriverContext->ShutdownCallbacks();

	if (GDriverContext->GetDeviceObject() != nullptr)
	{
		IoDeleteDevice(GDriverContext->GetDeviceObject());
		GDriverContext->SetDeviceObject(nullptr);
	}

	DestructItems(GDriverContext, 1);
	ExFreePool(GDriverContext);
	GDriverContext = nullptr;
}

namespace Driver
{
	namespace Private
	{
		NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS Status = STATUS_SUCCESS, uint32 Information = 0)
		{
			Irp->IoStatus.Status = Status;
			Irp->IoStatus.Information = Information;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return Status;
		}
	}
	
	NTSTATUS FLTAPI ArgusFilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
	{
		UNREFERENCED_PARAMETER(Flags);
		CleanupDriver();
		return STATUS_SUCCESS;
	}

	NTSTATUS ArgusCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
	{
		UNREFERENCED_PARAMETER(DeviceObject);
		return Driver::Private::CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}

	NTSTATUS ArgusRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
	{
		UNREFERENCED_PARAMETER(DeviceObject);
		return Driver::Private::CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}

	NTSTATUS ArgusWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp)
	{
		UNREFERENCED_PARAMETER(DeviceObject);
		return Driver::Private::CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}

	void ArgusUnload(PDRIVER_OBJECT DriverObject)
	{
		UNREFERENCED_PARAMETER(DriverObject);
		CleanupDriver();
	}
}

static NTSTATUS InitializeDriver(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "INFO", "driver.start",
		"version=%d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

	GDriverContext = static_cast<FDriverContext*>(ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(FDriverContext), 'erCk'));
	if (GDriverContext == nullptr)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	ConstructItem(GDriverContext, 'erCk');

	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Argus");
	//NTSTATUS Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, true, &DeviceObject);
	NTSTATUS Status = IoCreateDeviceSecure(DriverObject, 0, &DeviceName,
		FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, true,
		&SDDL_DEVOBJ_SYS_ALL, &Argus::Identity::ControlDeviceClassGuid, &DeviceObject);

	if (!NT_SUCCESS(Status))
	{
		CleanupDriver();
		return Status;
	}
	GDriverContext->SetDeviceObject(DeviceObject);

	DriverObject->MajorFunction[IRP_MJ_READ ] = Driver::ArgusRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Driver::ArgusWrite;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = Driver::ArgusCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE ] = Driver::ArgusCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Driver::ArgusDeviceControl;

	Status = GDriverContext->InitializeCallbacks(DriverObject, RegistryPath);
	if (!NT_SUCCESS(Status))
	{
		CleanupDriver();
		return Status;
	}

	UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\??\\Argus");
	Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
	if (!NT_SUCCESS(Status))
	{
		CleanupDriver();
		return Status;
	}
	GDriverContext->SetSymbolicLinkCreated(true);

	DriverObject->DriverUnload = Driver::ArgusUnload;
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	// File filtering is optional: a registration failure leaves the other
	// Argus callbacks and the control device available with mode None.
	Status = GDriverContext->GetMinifilter().InitStart(DriverObject);
	if (!NT_SUCCESS(Status))
	{
		Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN", "minifilter.start_failed",
			"status=0x%08X mode=none\n", static_cast<ULONG>(Status));
	}

	GDriverContext->GetMinifilter().SetFilterMode(EFilterMode::Passive);
	return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	return InitializeDriver(DriverObject, RegistryPath);
}
