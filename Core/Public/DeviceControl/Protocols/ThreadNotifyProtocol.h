#pragma once

#include "Core/Public/Notify/ThreadNotifyTypes.h"

namespace Argus::DeviceControl
{
	struct FThreadNotifyRecord
	{
		Argus::Notify::EThreadNotifyType NotifyType;
		// UTC time in 100-nanosecond intervals since January 1, 1601. Zero means unavailable.
		int64 CreatedAt;
		int64 ExitedAt;
		uint32 ProcessId;
		uint32 ThreadId;
	};

	static_assert(sizeof(FThreadNotifyRecord) == 32,
		"Thread notify record layout must match in kernel and user mode");
}