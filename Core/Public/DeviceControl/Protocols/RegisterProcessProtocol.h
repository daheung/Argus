#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::DeviceControl
{
	struct FInRegisterProcess
	{
		int64 CreatedAt;
		uint32 ProcessId;
	};
	static_assert(sizeof(FInRegisterProcess) == 16,
		"Register process request layout must match in kernel and user mode");

	enum class ERegisterProcessStatus : uint32
	{
		Success = 0,
		ProcessNotFound,
		AlreadyRegistered,
		AccessDenied,
		AllocationFailed,
	};

	struct FOutRegisterProcess
	{
		ERegisterProcessStatus Status;
	};
}
