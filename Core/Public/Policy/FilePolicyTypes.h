#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Policy/PolicyTypes.h"

namespace Argus::Policy
{
	enum class EMinifilterPolicySource: uint8
	{
		None,
		ProtectedProcessAssets,
		ArgusProcessAssets,
		ExplicitAssets,
	};

	enum class EMinifilterFileMatchMode : uint8
	{
		Path,
		Identity,
		AnyOf,
	};

	enum class EMinifilterPolicyScope : uint8
	{
		FileOnly,
		DirectoryTree,
		Volume,
	};

	enum class EMinifilterPolicyOperationFlags : uint32
	{
		None				= 0u,
		Read				= 1u << 0,
		Write				= 1u << 1,
		Create				= 1u << 2,
		Delete				= 1u << 3,
		Rename				= 1u << 4,
		QueryInformation	= 1u << 5,
		EnumDirectory		= 1u << 6,
		SetInformatin		= 1u << 7,
		SetSecurity			= 1u << 8,
	};

	enum class EVolumeDescriptorFields : uint32
	{
		None		 = 0,
		Guid		 = 1u << 0,
		SerialNumber = 1u << 1,
	};

	struct FVolumeDescriptor
	{
		uint32 ValidFields = 0;
		uint64 VolumeSerialNumber = 0;
		uint8 VolumeGuid[16]{};
	};

	struct FFileDescriptor
	{
	};

	struct FMinifilterPolicyRuleDescriptor
	{
		uint32 PolicyId;

		Argus::Policy::EPolicyRole Owner;
		uint32 OwnerProcessId;
		uint32 OwnerThreadId; // Optional

		FVolumeDescriptor VolumeDescriptor;
		FFileDescriptor FileDescriptor;

		EMinifilterPolicySource Source;
		EMinifilterPolicyScope Scope;
		EMinifilterPolicyOperationFlags DeniedOpFlags;
	};
}
