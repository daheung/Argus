#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Private/Minifilter/Descriptor/ObjectDescriptor.h"

namespace Argus::Minifilter::Volume
{
	// Called at PASSIVE_LEVEL during instance setup. The GUID field remains invalid
	// when the volume has no mount-manager GUID name.
	NTSTATUS CaptureGuid(PFLT_VOLUME Volume, Argus::Policy::FVolumeDescriptor& OutDescriptor) noexcept;
}

class FMinifilterVolumeContext
{
public:
	typedef TArray<EPoolType::NonPaged, FMinifilterVolumeDescriptor> FVolumeArray;

public:
	FMinifilterVolumeContext() noexcept
	{
		ExInitializePushLock(&VolumesLock);
	}

	FMinifilterVolumeContext(const FMinifilterVolumeContext&) = delete;
	FMinifilterVolumeContext& operator=(const FMinifilterVolumeContext&) = delete;

public:
	NTSTATUS RegisterVolume(PFLT_INSTANCE Instance, PFLT_VOLUME Volume, DEVICE_TYPE VolumeDeviceType, FLT_FILESYSTEM_TYPE FileSystemType) noexcept;

	void UnregisterVolume(PFLT_VOLUME Volume) noexcept;

	void RecordSetVolumeInformation(PCFLT_RELATED_OBJECTS FltObjects, FS_INFORMATION_CLASS InformationClass) noexcept;

	NTSTATUS GetVolumeDescriptor(PFLT_VOLUME Volume, FMinifilterVolumeDescriptor& OutDescriptor) noexcept;

private:
	void QueueDriveRootQuery(PFLT_INSTANCE Instance, PFLT_VOLUME Volume) noexcept;
	static VOID FLTAPI DriveRootQueryWorker(PFLT_GENERIC_WORKITEM WorkItem, PVOID FltObject, PVOID Context);

	FVolumeArray Volumes{ 'lVrA' };
	EX_PUSH_LOCK VolumesLock;
};
