#include "Kernel/Private/Minifilter/Minifilter.h"

#include <fltKernel.h>

namespace Driver
{
	NTSTATUS FLTAPI ArgusFilterUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
}

namespace
{
	FLT_PREOP_CALLBACK_STATUS FLTAPI PrePassThrough(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		UNREFERENCED_PARAMETER(Data);
		UNREFERENCED_PARAMETER(FltObjects);
		*CompletionContext = nullptr;
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreCreate(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreRead(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreWrite(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreQueryInformation(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreSetInformation(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreDirectoryControl(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreQuerySecurity(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreSetSecurity(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreQueryEa(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreSetEa(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreFileSystemControl(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreCleanup(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreClose(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreFlushBuffers(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	FLT_PREOP_CALLBACK_STATUS FLTAPI PreAcquireForSectionSynchronization(
		PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
	{
		return PrePassThrough(Data, FltObjects, CompletionContext);
	}

	NTSTATUS InstanceSetup(
		PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_SETUP_FLAGS Flags, 
		DEVICE_TYPE VolumeDeviceType, FLT_FILESYSTEM_TYPE VolumeFilesystemType
	) {
		UNREFERENCED_PARAMETER(FltObjects);
		UNREFERENCED_PARAMETER(Flags);
		UNREFERENCED_PARAMETER(VolumeDeviceType);

		return VolumeFilesystemType == FLT_FSTYPE_NTFS
			? STATUS_SUCCESS : STATUS_FLT_DO_NOT_ATTACH;
	}

	NTSTATUS InstanceQueryTeardown(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
	{
		UNREFERENCED_PARAMETER(FltObjects);
		UNREFERENCED_PARAMETER(Flags);
		return STATUS_SUCCESS;
	}

	void InstanceTeardownStart(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
	{
		UNREFERENCED_PARAMETER(FltObjects);
		UNREFERENCED_PARAMETER(Flags);
	}

	void InstanceTeardownComplete(PCFLT_RELATED_OBJECTS FltObjects, FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
	{
		UNREFERENCED_PARAMETER(FltObjects);
		UNREFERENCED_PARAMETER(Flags);
	}

	const FLT_OPERATION_REGISTRATION Operations[] =
	{
		{ IRP_MJ_CREATE				, 0, PreCreate, nullptr },
		{ IRP_MJ_READ				, 0, PreRead, nullptr },
		{ IRP_MJ_WRITE				, 0, PreWrite, nullptr },
		{ IRP_MJ_QUERY_INFORMATION	, 0, PreQueryInformation, nullptr },
		{ IRP_MJ_SET_INFORMATION	, 0, PreSetInformation, nullptr },
		{ IRP_MJ_DIRECTORY_CONTROL	, 0, PreDirectoryControl, nullptr },
		{ IRP_MJ_QUERY_SECURITY		, 0, PreQuerySecurity, nullptr },
		{ IRP_MJ_SET_SECURITY		, 0, PreSetSecurity, nullptr },
		{ IRP_MJ_QUERY_EA			, 0, PreQueryEa, nullptr },
		{ IRP_MJ_SET_EA				, 0, PreSetEa, nullptr },
		{ IRP_MJ_FILE_SYSTEM_CONTROL, 0, PreFileSystemControl, nullptr },
		{ IRP_MJ_CLEANUP			, 0, PreCleanup, nullptr },
		{ IRP_MJ_CLOSE				, 0, PreClose, nullptr },
		{ IRP_MJ_FLUSH_BUFFERS		, 0, PreFlushBuffers, nullptr },
		{ IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION, 0, PreAcquireForSectionSynchronization, nullptr },
		{ IRP_MJ_OPERATION_END },
	};
	const FLT_REGISTRATION Registration =
	{
		sizeof(FLT_REGISTRATION),
		FLT_REGISTRATION_VERSION,
		0,
		nullptr,
		Operations,
		Driver::ArgusFilterUnload,
		InstanceSetup,
		InstanceQueryTeardown,
		InstanceTeardownStart,
		InstanceTeardownComplete,		
	};
}

NTSTATUS FMinifilter::Start(PDRIVER_OBJECT DriverObject) noexcept
{
	if (bStartAttempted)
	{
		return STATUS_INVALID_DEVICE_STATE;
	}
	bStartAttempted = true;

	NTSTATUS Status = FltRegisterFilter(DriverObject, &Registration, &Filter);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	// FltRegisterFilter succeeded, so failure in FltStartFiltering must unregister Filter.
	FilterState.store(static_cast<uint32>(EFilterState::Registered), Atomic::EMemoryOrder::Release);
	FilterMode.store(static_cast<uint32>(EFilterMode::Pass), Atomic::EMemoryOrder::Release);

	Status = FltStartFiltering(Filter);
	if (!NT_SUCCESS(Status))
	{
		this->UnregisterFilter();
		return Status;
	}
	
	return Status;
}

void FMinifilter::UnregisterFilter() noexcept
{
	CORE_CHECK(KeGetCurrentIrql() <= APC_LEVEL);

	uint32 ExpectedState = static_cast<uint32>(EFilterState::Registered);
	if (!FilterState.compare_exchange_strong(ExpectedState, static_cast<uint32>(EFilterState::Unregistering), Atomic::EMemoryOrder::AcqRel))
	{
		if (ExpectedState == static_cast<uint32>(EFilterState::Unregistering))
		{
			KeWaitForSingleObject(&UnregisterComplete, Executive, KernelMode, FALSE, nullptr);
		}
		return;
	}

	CORE_CHECK(Filter != nullptr);
	FltUnregisterFilter(Filter);
	Filter = nullptr;

	FilterMode.store(static_cast<uint32>(EFilterMode::None), Atomic::EMemoryOrder::Release);
	FilterState.store(static_cast<uint32>(EFilterState::Unregistered), Atomic::EMemoryOrder::Release);
	KeSetEvent(&UnregisterComplete, IO_NO_INCREMENT, FALSE);
}

NTSTATUS FMinifilter::SetFilterMode(EFilterMode NewMode) noexcept
{
	if (NewMode == EFilterMode::None)
	{
		this->UnregisterFilter();
		return STATUS_SUCCESS;
	}
	if (GetFilterState() != EFilterState::Registered)
	{
		return STATUS_DEVICE_NOT_READY;
	}
	if (NewMode != EFilterMode::Pass)
	{
		// File event collection and an enforcement policy are not wired yet.
		return STATUS_NOT_SUPPORTED;
	}

	FilterMode.store(static_cast<uint32>(NewMode), Atomic::EMemoryOrder::Release);
	if (GetFilterState() != EFilterState::Registered)
	{
		FilterMode.store(static_cast<uint32>(EFilterMode::None), Atomic::EMemoryOrder::Release);
		return STATUS_DEVICE_NOT_READY;
	}
	return STATUS_SUCCESS;
}
