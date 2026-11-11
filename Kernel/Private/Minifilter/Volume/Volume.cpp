#include "Kernel/Private/Minifilter/Volume/Volume.h"
#include "Kernel/Private/Logging.h"

namespace
{
	constexpr ULONG VolumeGuidTag	 = 'gVrA';
	constexpr ULONG VolumeNameTag	 = 'nVrA';
	constexpr ULONG DriveRootWorkTag = 'wVrA';
	constexpr ULONG SymlinkQueryTag  = 'sVrA';
	constexpr USHORT GuidTextChars = 38; // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	constexpr WCHAR UnavailableDriveRoot[] = L"Unavailable";

	struct FDriveRootQueryContext
	{
		FMinifilterVolumeContext* Owner;
		PFLT_VOLUME Volume;
	};

	void CaptureDeviceRoot(PFLT_VOLUME Volume, FString<EPoolType::NonPaged>& OutDeviceRoot) noexcept
	{
		ULONG RequiredBytes = 0;
		if (FltGetVolumeName(Volume, nullptr, &RequiredBytes) != STATUS_BUFFER_TOO_SMALL ||
			RequiredBytes == 0 || RequiredBytes > MAXUSHORT)
		{
			return;
		}

		PWCH Buffer = static_cast<PWCH>(ExAllocatePool2(POOL_FLAG_PAGED,
			static_cast<size_t>(RequiredBytes) + sizeof(WCHAR), VolumeNameTag));
		if (Buffer == nullptr)
		{
			return;
		}

		UNICODE_STRING Name{};
		Name.Buffer = Buffer;
		Name.MaximumLength = static_cast<USHORT>(RequiredBytes);
		if (NT_SUCCESS(FltGetVolumeName(Volume, &Name, nullptr)) &&
			Name.Length != 0 && (Name.Length % sizeof(WCHAR)) == 0)
		{
			int32 CharacterCount = Name.Length / sizeof(WCHAR);
			if (Buffer[CharacterCount - 1] != L'\\')
			{
				Buffer[CharacterCount++] = L'\\';
			}
			OutDeviceRoot.InitializeWithLength(Buffer, CharacterCount);
		}
		ExFreePoolWithTag(Buffer, VolumeNameTag);
	}

	NTSTATUS QueryVolumeDeviceName(PFLT_VOLUME Volume,
		FString<EPoolType::NonPaged>& OutDeviceName) noexcept
	{
		if (Volume == nullptr)
		{
			return STATUS_INVALID_PARAMETER;
		}

		ULONG RequiredBytes = 0;
		NTSTATUS Status = FltGetVolumeName(Volume, nullptr, &RequiredBytes);
		if (Status != STATUS_BUFFER_TOO_SMALL)
		{
			return NT_SUCCESS(Status) ? STATUS_INVALID_BUFFER_SIZE : Status;
		}
		if (RequiredBytes == 0 || RequiredBytes > MAXUSHORT)
		{
			return STATUS_INVALID_BUFFER_SIZE;
		}

		auto* Buffer = static_cast<PWCH>(ExAllocatePool2(
			POOL_FLAG_PAGED, RequiredBytes, VolumeNameTag));
		if (Buffer == nullptr)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		UNICODE_STRING Name{};
		Name.Buffer = Buffer;
		Name.MaximumLength = static_cast<USHORT>(RequiredBytes);
		Status = FltGetVolumeName(Volume, &Name, nullptr);
		if (NT_SUCCESS(Status))
		{
			if (Name.Length == 0 || (Name.Length % sizeof(WCHAR)) != 0)
			{
				Status = STATUS_INVALID_BUFFER_SIZE;
			}
			else if (!OutDeviceName.InitializeWithLength(Buffer,
				Name.Length / sizeof(WCHAR)))
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}

		ExFreePoolWithTag(Buffer, VolumeNameTag);
		return Status;
	}

	NTSTATUS QueryObjectManagerDriveRoot(PCUNICODE_STRING DeviceName,
		FString<EPoolType::NonPaged>& OutDriveRoot) noexcept
	{
		if (DeviceName == nullptr || DeviceName->Buffer == nullptr ||
			DeviceName->Length == 0 || (DeviceName->Length % sizeof(WCHAR)) != 0)
		{
			return STATUS_INVALID_PARAMETER;
		}
		if (KeGetCurrentIrql() != PASSIVE_LEVEL)
		{
			return STATUS_INVALID_DEVICE_STATE;
		}

		// A longer target cannot be the same device name.
		const USHORT TargetBytes = DeviceName->Length <= MAXUSHORT - sizeof(WCHAR)
			? static_cast<USHORT>(DeviceName->Length + sizeof(WCHAR)) : DeviceName->Length;
		auto* TargetBuffer = static_cast<PWCH>(ExAllocatePool2(
			POOL_FLAG_PAGED, TargetBytes, SymlinkQueryTag));
		if (TargetBuffer == nullptr)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		// This explicitly selects the global DOS device namespace (\GLOBAL??).
		WCHAR LinkNameBuffer[] = L"\\DosDevices\\Global\\A:";
		constexpr ULONG LetterIndex = RTL_NUMBER_OF(LinkNameBuffer) - 3;
		UNICODE_STRING LinkName{};
		RtlInitUnicodeString(&LinkName, LinkNameBuffer);
		NTSTATUS Status = STATUS_NOT_FOUND;

		for (WCHAR Letter = L'A'; Letter <= L'Z'; ++Letter)
		{
			LinkNameBuffer[LetterIndex] = Letter;
			OBJECT_ATTRIBUTES Attributes{};
			InitializeObjectAttributes(&Attributes, &LinkName,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);

			HANDLE LinkHandle = nullptr;
			const NTSTATUS OpenStatus = ZwOpenSymbolicLinkObject(&LinkHandle, SYMBOLIC_LINK_QUERY, &Attributes);
			if (!NT_SUCCESS(OpenStatus))
			{
				continue;
			}

			UNICODE_STRING Target{};
			Target.Buffer = TargetBuffer;
			Target.MaximumLength = TargetBytes;
			const NTSTATUS QueryStatus = ZwQuerySymbolicLinkObject(LinkHandle, &Target, nullptr);

			ZwClose(LinkHandle);

			const bool bIsTargetVolume = RtlEqualUnicodeString(&Target, DeviceName, TRUE);
			if (!NT_SUCCESS(QueryStatus) || !bIsTargetVolume)
			{
				continue;
			}

			const WCHAR Root[4]{ Letter, L':', L'\\', L'\0' };
			Status = OutDriveRoot.InitializeWithLength(Root, 3)
				? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;

			break;
		}

		ExFreePoolWithTag(TargetBuffer, SymlinkQueryTag);
		return Status;
	}

	bool CloneVolumeDescriptor(const FMinifilterVolumeDescriptor& Source,
		FMinifilterVolumeDescriptor& Destination) noexcept
	{
		Destination.VolumeContext		  = Source.VolumeContext;
		Destination.Identity			  = Source.Identity;
		Destination.FileSystemType		  = Source.FileSystemType;
		Destination.VolumeDeviceType	  = Source.VolumeDeviceType;
		Destination.DeviceCharacteristics = Source.DeviceCharacteristics;
		Destination.SectorSize			  = Source.SectorSize;
		Destination.bPropertiesAvailable  = Source.bPropertiesAvailable;
		
		const bool bDeviceRootInitSucceed = Destination.DeviceRoot.InitializeWithLength(*Source.DeviceRoot, Source.DeviceRoot.Len());
		if (Source.DeviceRoot.Len() != 0 && !bDeviceRootInitSucceed)
		{
			return false;
		}
		
		const bool bDriveRootInitSucceed = Destination.DriveRoot.InitializeWithLength(*Source.DriveRoot, Source.DriveRoot.Len());
		if (Source.DriveRoot.Len() != 0 && !bDriveRootInitSucceed)
		{
			return false;
		}

		return true;
	}

	void LogVolumeEvent(const char* Event, const FMinifilterVolumeDescriptor& Descriptor) noexcept
	{
		char GuidText[37]{};
		const char* VolumeGuid = "<unavailable>";
		if ((Descriptor.Identity.ValidFields &
			static_cast<uint32>(Argus::Policy::EVolumeDescriptorFields::Guid)) != 0)
		{
			GUID Guid{};
			RtlCopyMemory(&Guid, Descriptor.Identity.VolumeGuid, sizeof(Guid));
			if (NT_SUCCESS(RtlStringCchPrintfA(GuidText, RTL_NUMBER_OF(GuidText),
				"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
				Guid.Data1	 , Guid.Data2	, Guid.Data3,
				Guid.Data4[0], Guid.Data4[1], Guid.Data4[2], Guid.Data4[3],
				Guid.Data4[4], Guid.Data4[5], Guid.Data4[6], Guid.Data4[7])))
			{
				VolumeGuid = GuidText;
			}
		}

		Argus::KernelLogging::Write(DPFLTR_INFO_LEVEL, "INFO", Event,
			"volume=%p volume_guid=%s\n"
			"\tdevice_root = \"%ws\" drive_root=\"%ws\" filesystem_type=%u device_type=%lu properties_available=%u\n",
			Descriptor.VolumeContext, VolumeGuid,
			Descriptor.DeviceRoot.Len() != 0 ? *Descriptor.DeviceRoot : L"<unavailable>",
			Descriptor.DriveRoot .Len() != 0 ? *Descriptor.DriveRoot  : L"<unavailable>",
			static_cast<unsigned int>(Descriptor.FileSystemType),
			static_cast<unsigned long>(Descriptor.VolumeDeviceType),
			Descriptor.bPropertiesAvailable ? 1u : 0u);
	}
}

NTSTATUS Argus::Minifilter::Volume::CaptureGuid(
	PFLT_VOLUME Volume, Argus::Policy::FVolumeDescriptor& OutDescriptor) noexcept
{
	if (Volume == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	ULONG RequiredBytes = 0;
	NTSTATUS Status = FltGetVolumeGuidName(Volume, nullptr, &RequiredBytes);
	if (Status != STATUS_BUFFER_TOO_SMALL)
	{
		return Status;
	}
	if (RequiredBytes < GuidTextChars * sizeof(WCHAR) || RequiredBytes > MAXUSHORT)
	{
		return STATUS_INVALID_BUFFER_SIZE;
	}

	auto* Buffer = static_cast<PWCH>(ExAllocatePool2(POOL_FLAG_PAGED, RequiredBytes, VolumeGuidTag));
	if (Buffer == nullptr)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	UNICODE_STRING Name{};
	Name.Buffer = Buffer;
	Name.MaximumLength = static_cast<USHORT>(RequiredBytes);
	Status = FltGetVolumeGuidName(Volume, &Name, nullptr);
	if (NT_SUCCESS(Status) && (Name.Length % sizeof(WCHAR)) == 0)
	{
		const USHORT CharacterCount = Name.Length / sizeof(WCHAR);
		Status = STATUS_INVALID_PARAMETER;
		for (USHORT Index = 0; Index + GuidTextChars <= CharacterCount; ++Index)
		{
			if (Buffer[Index] != L'{' || Buffer[Index + GuidTextChars - 1] != L'}')
			{
				continue;
			}

			UNICODE_STRING GuidText{};
			GuidText.Buffer = Buffer + Index;
			GuidText.Length = GuidTextChars * sizeof(WCHAR);
			GuidText.MaximumLength = GuidText.Length;
			GUID Guid{};
			Status = RtlGUIDFromString(&GuidText, &Guid);
			if (NT_SUCCESS(Status))
			{
				static_assert(sizeof(Guid) == sizeof(OutDescriptor.VolumeGuid));
				RtlCopyMemory(OutDescriptor.VolumeGuid, &Guid, sizeof(Guid));
				OutDescriptor.ValidFields |= static_cast<uint32>(Argus::Policy::EVolumeDescriptorFields::Guid);
			}
			break;
		}
	}
	else if (NT_SUCCESS(Status))
	{
		Status = STATUS_INVALID_PARAMETER;
	}

	ExFreePoolWithTag(Buffer, VolumeGuidTag);
	return Status;
}

NTSTATUS FMinifilterVolumeContext::RegisterVolume(PFLT_INSTANCE Instance, PFLT_VOLUME Volume,
	DEVICE_TYPE VolumeDeviceType, FLT_FILESYSTEM_TYPE FileSystemType) noexcept
{
	CORE_CHECK(KeGetCurrentIrql() == PASSIVE_LEVEL);
	if (Instance == nullptr || Volume == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}

	FMinifilterVolumeDescriptor VolumeDescriptor{};
	VolumeDescriptor.VolumeContext	  = Volume;
	VolumeDescriptor.FileSystemType   = FileSystemType;
	VolumeDescriptor.VolumeDeviceType = VolumeDeviceType;
	Argus::Minifilter::Volume::CaptureGuid(Volume, VolumeDescriptor.Identity);
	CaptureDeviceRoot(Volume, VolumeDescriptor.DeviceRoot);

	FLT_VOLUME_PROPERTIES Properties{};
	ULONG PropertiesBytes = 0;
	const NTSTATUS PropertiesStatus = FltGetVolumeProperties(Volume,
		&Properties, sizeof(Properties), &PropertiesBytes);
	if (PropertiesStatus == STATUS_SUCCESS || PropertiesStatus == STATUS_BUFFER_OVERFLOW)
	{
		VolumeDescriptor.DeviceCharacteristics = Properties.DeviceCharacteristics;
		VolumeDescriptor.SectorSize			   = Properties.SectorSize;
		VolumeDescriptor.bPropertiesAvailable  = true;
	}

	if (!VolumeDescriptor.DriveRoot.InitializeWithLength(
		UnavailableDriveRoot, RTL_NUMBER_OF(UnavailableDriveRoot) - 1))
	{
		Argus::KernelLogging::Write(DPFLTR_ERROR_LEVEL, "ERROR", "volume.register_failed",
			"volume=%p status=0x%08X\n", Volume,
			static_cast<unsigned int>(STATUS_INSUFFICIENT_RESOURCES));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	NTSTATUS Status = STATUS_SUCCESS;

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&VolumesLock);

	for (int32 Index = 0; Index < Volumes.Num(); ++Index)
	{
		if (Volumes[Index].VolumeContext == Volume)
		{
			Status = STATUS_OBJECT_NAME_COLLISION;
			break;
		}
	}
	if (NT_SUCCESS(Status) && Volumes.Push(MoveTemp(VolumeDescriptor)) == INDEX_NONE)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
	}

	ExReleasePushLockExclusive(&VolumesLock);
	KeLeaveCriticalRegion();

	if (NT_SUCCESS(Status))
	{
		QueueDriveRootQuery(Instance, Volume);
	}
	else
	{
		Argus::KernelLogging::Write(DPFLTR_ERROR_LEVEL, "ERROR", "volume.register_failed",
			"volume=%p status=0x%08X\n", Volume, static_cast<unsigned int>(Status));
	}
	return Status;
}

void FMinifilterVolumeContext::QueueDriveRootQuery(PFLT_INSTANCE Instance, PFLT_VOLUME Volume) noexcept
{
	auto* Query = static_cast<FDriveRootQueryContext*>(
		ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(FDriveRootQueryContext), DriveRootWorkTag));
	if (Query == nullptr)
	{
		return;
	}

	Query->Owner  = this;
	Query->Volume = Volume;

	PFLT_GENERIC_WORKITEM WorkItem = FltAllocateGenericWorkItem();
	if (WorkItem == nullptr)
	{
		ExFreePoolWithTag(Query, DriveRootWorkTag);
		return;
	}

	NTSTATUS Status = FltObjectReference(Query->Volume);
	if (NT_SUCCESS(Status))
	{
		Status = FltQueueGenericWorkItem(WorkItem, Instance,
			DriveRootQueryWorker, DelayedWorkQueue, Query);
		if (!NT_SUCCESS(Status))
		{
			FltObjectDereference(Query->Volume);
		}
	}
	if (!NT_SUCCESS(Status))
	{
		FltFreeGenericWorkItem(WorkItem);
		ExFreePoolWithTag(Query, DriveRootWorkTag);
		Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN",
			"volume.drive_root_query_enqueue_failed",
			"volume=%p status=0x%08X\n", Volume,
			static_cast<unsigned int>(Status));
	}
}

VOID FLTAPI FMinifilterVolumeContext::DriveRootQueryWorker(
	PFLT_GENERIC_WORKITEM WorkItem, PVOID FltObject, PVOID Context)
{
	UNREFERENCED_PARAMETER(FltObject);
	auto* Query = static_cast<FDriveRootQueryContext*>(Context);
	if (Query != nullptr)
	{
		FString<EPoolType::NonPaged> DeviceNameText{};
		const NTSTATUS DeviceStatus = QueryVolumeDeviceName(Query->Volume, DeviceNameText);
		UNICODE_STRING DeviceName{};
		if (NT_SUCCESS(DeviceStatus))
		{
			DeviceName.Buffer = const_cast<PWCH>(*DeviceNameText);
			DeviceName.Length = static_cast<USHORT>(DeviceNameText.Len() * sizeof(WCHAR));
			DeviceName.MaximumLength = DeviceName.Length;
		}
		FString<EPoolType::NonPaged> DriveRoot{};
		const NTSTATUS Status = NT_SUCCESS(DeviceStatus)
			? QueryObjectManagerDriveRoot(&DeviceName, DriveRoot) : DeviceStatus;
		if (!NT_SUCCESS(Status) && Status != STATUS_NOT_FOUND)
		{
			Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN",
				"volume.drive_root_query_failed",
				"volume=%p stage=%s status=0x%08X device_name_bytes=%u apcs_disabled=%u\n",
				Query->Volume, NT_SUCCESS(DeviceStatus) ? "object_manager" : "flt_get_volume_name",
				static_cast<unsigned int>(Status),
				static_cast<unsigned int>(DeviceName.Length),
				KeAreAllApcsDisabled() ? 1u : 0u);
		}

		if (NT_SUCCESS(Status))
		{
			KeEnterCriticalRegion();
			ExAcquirePushLockExclusive(&Query->Owner->VolumesLock);
			
			for (int32 Index = 0; Index < Query->Owner->Volumes.Num(); ++Index)
			{
				auto& Descriptor = Query->Owner->Volumes[Index];
				if (Descriptor.VolumeContext == Query->Volume)
				{
					FString<EPoolType::NonPaged>::Swap(Descriptor.DriveRoot, DriveRoot);
					break;
				}
			}
			
			ExReleasePushLockExclusive(&Query->Owner->VolumesLock);
			KeLeaveCriticalRegion();
		}

		FMinifilterVolumeDescriptor LogDescriptor{};
		const NTSTATUS SnapshotStatus = Query->Owner->GetVolumeDescriptor(
			Query->Volume, LogDescriptor);
		if (NT_SUCCESS(SnapshotStatus))
		{
			LogVolumeEvent("volume.registered", LogDescriptor);
		}
		else if (SnapshotStatus != STATUS_NOT_FOUND)
		{
			Argus::KernelLogging::Write(DPFLTR_WARNING_LEVEL, "WARN",
				"volume.log_snapshot_incomplete",
				"volume=%p status=0x%08X\n", Query->Volume,
				static_cast<unsigned int>(SnapshotStatus));
		}

		FltObjectDereference(Query->Volume);
		ExFreePoolWithTag(Query, DriveRootWorkTag);
	}

	FltFreeGenericWorkItem(WorkItem);
}

void FMinifilterVolumeContext::UnregisterVolume(PFLT_VOLUME Volume) noexcept
{
	CORE_CHECK(KeGetCurrentIrql() == PASSIVE_LEVEL);

	FMinifilterVolumeDescriptor RemovedDescriptor{};
	bool bRemoved = false;

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&VolumesLock);

	for (int32 Index = 0; Index < Volumes.Num(); ++Index)
	{
		if (Volumes[Index].VolumeContext == Volume)
		{
			RemovedDescriptor = MoveTemp(Volumes[Index]);
			Volumes.RemoveAt(Index, 1, false);
			bRemoved = true;
			break;
		}
	}

	ExReleasePushLockExclusive(&VolumesLock);
	KeLeaveCriticalRegion();

	if (bRemoved)
	{
		LogVolumeEvent("volume.unregistered", RemovedDescriptor);
	}
}

void FMinifilterVolumeContext::RecordSetVolumeInformation(
	PCFLT_RELATED_OBJECTS FltObjects, FS_INFORMATION_CLASS InformationClass) noexcept
{
	// The settable information classes do not map to fields currently cached here.
	UNREFERENCED_PARAMETER(InformationClass);
	if (FltObjects == nullptr || FltObjects->Instance == nullptr ||
		KeGetCurrentIrql() > APC_LEVEL)
	{
		return;
	}

	PFLT_VOLUME Volume = nullptr;
	if (!NT_SUCCESS(FltGetVolumeFromInstance(FltObjects->Instance, &Volume)))
	{
		return;
	}

	FLT_VOLUME_PROPERTIES Properties{};
	ULONG PropertiesBytes = 0;
	const NTSTATUS PropertiesStatus = FltGetVolumeProperties(
		Volume, &Properties, sizeof(Properties), &PropertiesBytes);
	if (PropertiesStatus == STATUS_SUCCESS || 
		PropertiesStatus == STATUS_BUFFER_OVERFLOW)
	{
		KeEnterCriticalRegion();
		ExAcquirePushLockExclusive(&VolumesLock);

		for (int32 Index = 0; Index < Volumes.Num(); ++Index)
		{
			if (Volumes[Index].VolumeContext == Volume)
			{
				Volumes[Index].DeviceCharacteristics = Properties.DeviceCharacteristics;
				Volumes[Index].SectorSize = Properties.SectorSize;
				Volumes[Index].bPropertiesAvailable = true;
				break;
			}
		}

		ExReleasePushLockExclusive(&VolumesLock);
		KeLeaveCriticalRegion();
	}

	FltObjectDereference(Volume);
}

NTSTATUS FMinifilterVolumeContext::GetVolumeDescriptor(
	PFLT_VOLUME Volume, FMinifilterVolumeDescriptor& OutDescriptor) noexcept
{
	if (Volume == nullptr)
	{
		return STATUS_INVALID_PARAMETER;
	}
	if (KeGetCurrentIrql() > APC_LEVEL)
	{
		return STATUS_INVALID_DEVICE_STATE;
	}

	NTSTATUS Status = STATUS_NOT_FOUND;
	FMinifilterVolumeDescriptor Snapshot{};

	KeEnterCriticalRegion();
	ExAcquirePushLockShared(&VolumesLock);

	for (int32 Index = 0; Index < Volumes.Num(); ++Index)
	{
		if (Volumes[Index].VolumeContext == Volume)
		{
			Status = CloneVolumeDescriptor(Volumes[Index], Snapshot)
				? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
	}

	ExReleasePushLockShared(&VolumesLock);
	KeLeaveCriticalRegion();
	
	if (NT_SUCCESS(Status))
	{
		OutDescriptor = MoveTemp(Snapshot);
	}

	return Status;
}

