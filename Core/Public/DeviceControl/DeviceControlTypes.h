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
		uint64 Offset;
		uint64 Bytes;
	};
	static_assert(sizeof(FStringDescriptor) == 16);
	static_assert(offsetof(FStringDescriptor, Offset) == 0);
	static_assert(offsetof(FStringDescriptor, Bytes) == 8);
}
