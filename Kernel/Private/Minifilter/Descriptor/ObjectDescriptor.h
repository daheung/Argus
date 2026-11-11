#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Kernel/Public/Containers/KrnlString.h"
#include "Kernel/Public/Misc/KernelDefines.h"

struct FMinifilterVolumeDescriptor
{
public:
	FMinifilterVolumeDescriptor() noexcept = default;
	FMinifilterVolumeDescriptor(const FMinifilterVolumeDescriptor&) = delete;
	FMinifilterVolumeDescriptor& operator=(const FMinifilterVolumeDescriptor&) = delete;
	FMinifilterVolumeDescriptor(FMinifilterVolumeDescriptor&&) noexcept = default;
	FMinifilterVolumeDescriptor& operator=(FMinifilterVolumeDescriptor&& Other) noexcept
	{
		if (this != &Other)
		{
			VolumeContext		  = Other.VolumeContext;
			Identity			  = Other.Identity;
			FileSystemType		  = Other.FileSystemType;
			VolumeDeviceType	  = Other.VolumeDeviceType;
			DeviceCharacteristics = Other.DeviceCharacteristics;
			SectorSize			  = Other.SectorSize;
			bPropertiesAvailable  = Other.bPropertiesAvailable;
			FString<EPoolType::NonPaged>::Swap(DeviceRoot, Other.DeviceRoot);
			FString<EPoolType::NonPaged>::Swap(DriveRoot , Other.DriveRoot);
		}

		return *this;
	}

public:
	PFLT_VOLUME VolumeContext = nullptr;

	Argus::Policy::FVolumeDescriptor Identity;
	FString<EPoolType::NonPaged> DeviceRoot; // Runtime NT device root.
	FString<EPoolType::NonPaged> DriveRoot;  // DOS drive root, or "Unavailable".

	FLT_FILESYSTEM_TYPE FileSystemType{};
	DEVICE_TYPE VolumeDeviceType{};
	
	uint32 DeviceCharacteristics = 0;
	uint16 SectorSize = 0;

	bool bPropertiesAvailable = false;
};

template <>
struct TIsTriviallyRelocatable<FMinifilterVolumeDescriptor>
{
	static constexpr bool Value = true;
};

class FMinifilterFileDescriptor
{

};
