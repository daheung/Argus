#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::DeviceControl
{
	enum class EStringStatus : uint8
	{
		NotApplicable,
		Captured,
		Unavailable,
		AllocationFailed,
	};

	enum class EProcessorMode : uint8
	{
		Unknown,
		UserMode,
		KernelMode,
	};

	struct FStringDescriptor
	{
		EStringStatus Status;
		uint64 Offset;
		uint64 Bytes;
	};
}