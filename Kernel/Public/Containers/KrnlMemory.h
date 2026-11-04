#pragma once
#include "Core/Public/Misc/Defines.h"

#if defined(PLATFORM_WDM) || defined(PLATFORM_KMDF)

#include "Kernel/Public/Misc/KernelDefines.h"

namespace Argus::Memory
{
	struct FKrnlMemory
	{
		// PoolFlags must select paged or nonpaged pool. The caller owns the result.
		NODISCARD static void* Malloc(POOL_FLAGS PoolFlags, SIZE_T Size, ULONG Tag) noexcept;

		static void Free(void* Allocation) noexcept;
	};
}

using FKrnlMemory = Argus::Memory::FKrnlMemory;

#endif
