#pragma once

#include "Core/Public/Notify/ImageNotifyTypes.h"

namespace Argus::DeviceControl
{
	// One output slot contains the record followed by up to 256 UTF-16 bytes.
	struct FImageNotifyRecord
	{
		// UTC time in 100-nanosecond intervals since January 1, 1601. Zero means unavailable.
		int64 LoadedAt;
		uint32 ProcessId;
		uint64 ImageSize;
		uint64 ImageBaseAddress;
		uint32 ImageFileNameBytes;
	};

	struct FImageNotifySlot
	{
		FImageNotifyRecord Record;
		uint8 ImageFileName[Argus::Notify::ImageNotifyMaxFileNameBytes];
	};

	static_assert(sizeof(FImageNotifyRecord) == 40,
		"Image notify record layout must match in kernel and user mode");
	static_assert(sizeof(FImageNotifySlot) ==
		sizeof(FImageNotifyRecord) + Argus::Notify::ImageNotifyMaxFileNameBytes,
		"Image notify slots must have a fixed wire size");
}
