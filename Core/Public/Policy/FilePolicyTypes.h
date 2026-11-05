#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Policy/PolicyTypes.h"

namespace Argus::Policy
{
	enum class EFileObjectPolicySource: uint8
	{
		None,
		ProtectedProcessAsset,
		ArgusProcessAsset,
		ExplicitAsset,
	};

	enum class EFileObjectPolicyScope : uint8
	{
		FileOnly,
		DirectoryTree,
	};

	enum class EFileObjecyPolicyOperationFlags : uint32
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

	enum class EFileObjectMatchMode : uint8
	{
		Path,
		Identity,
		AnyOf,
	};

	struct FFileObjectDescriptor
	{
		uint64 FileObjectId;
		uint64 VolumeSerialNumber;
	};

	struct FFileObjectPolicyRuleDescriptor
	{
		uint32 PolicyId;

		Argus::Policy::EPolicyRole Owner;
		uint32 OwnerProcessId;
		uint32 OwnerThreadId;

		FFileObjectDescriptor FileDescriptor;

		EFileObjectPolicySource Source;
		EFileObjectPolicyScope Scope;
		EFileObjecyPolicyOperationFlags DeniedOpFlags;
	};
}