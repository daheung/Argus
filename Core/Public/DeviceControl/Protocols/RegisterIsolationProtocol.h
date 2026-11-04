#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::DeviceControl
{
	enum class EInIsolationFlags : uint32
	{
		None					    =  0u,
		DenyChildProcessCreation	=  1u << 0,
		RestrictProcessHandleRights =  1u << 1,
		RestrictThreadHandleRights  =  1u << 2,

		All							= (1u << 0) | (1u << 1) | (1u << 2)
	};

	struct FInRegisterIsolationProcess
	{
		int64 CreatedAt;
		uint32 ProcessId;

		EInIsolationFlags Flags;
	};

	static_assert(sizeof(FInRegisterIsolationProcess) == 16,
		"Register process request layout must match in kernel and user mode");

	enum class ERegisterIsolationProcessStatus : uint32
	{
		Success = 0,
		ProcessNotFound,
		AlreadyRegistered,
		AccessDenied,
		AllocationFailed,
	};

	struct FOutRegisterIsolationProcess
	{
		ERegisterIsolationProcessStatus Status;
	};
}
