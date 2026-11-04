#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::Notify
{
	enum class ERegistryOperationType : uint16
	{
		Unknown,
		CreateKey,
		OpenKey,
		DeleteKey,
		RenameKey,
		SetValue,
		DeleteValue,
		QueryKey,
		QueryValue,
		QueryMultipleValues,
		EnumerateKey,
		EnumerateValue,
		SetKeyInformation,
		QueryKeySecurity,
		SetKeySecurity,
		FlushKey,
		LoadKey,
		UnloadKey,
		RestoreKey,
		SaveKey,
		ReplaceKey,
		CloseKey,
		QueryKeyName,
		SaveMergedKey,
	};

	// A requester may have more than one role; combine these values as bits.
	enum class ERegistryRequesterRole : uint8
	{
		None = 0,
		Agent = 1u << 0,
		Protected = 1u << 1,
		Isolated = 1u << 2,
	};

	// Reserved for classifying a registry target by a future key-path policy.
	// A key or value does not acquire a role merely from its requester's role.
	enum class ERegistryTargetRole : uint8
	{
		None = 0,
		Agent = 1u << 0,
		Protected = 1u << 1,
		Isolated = 1u << 2,
	};

	enum class ERegistryNotifyPhase : uint8
	{
		Unknown,
		Pre,
		Post,
	};

	enum class ERegistryDecision : uint8
	{
		Unknown,
		Allowed,
		Blocked,
	};

	enum class ERegistryNameStatus : uint8
	{
		NotApplicable,
		Captured,
		Unavailable,
		AllocationFailed,
	};
}
