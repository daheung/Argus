#pragma once

#include "Core/Public/Notify/ThreadNotifyTypes.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Kernel/Public/Misc/KernelDefines.h"

namespace Argus::Notify
{
	struct FThreadNotifyInfo
	{
		EThreadNotifyType NotifyType = EThreadNotifyType::Unknown;
		LARGE_INTEGER CreatedAt{};
		LARGE_INTEGER ExitedAt{};
		uint32 ProcessId = 0;
		uint32 ThreadId = 0;
		uint32 ExitCode = 0;
	};
}

template <>
struct TIsTriviallyRelocatable<Argus::Notify::FThreadNotifyInfo>
{
	static constexpr bool Value = true;
};
