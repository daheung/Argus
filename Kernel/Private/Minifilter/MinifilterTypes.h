#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"

namespace Argus::Minifilter
{
	struct FMinifilterContext
	{
		uint64 ContextId;
		uint32 ContextProcessId;
		int64  ContextProcessCreatedAt;
		int64  ContextCreatedAt;
	};
}

