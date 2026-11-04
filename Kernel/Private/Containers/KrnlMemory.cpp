#include "Kernel/Public/Containers/KrnlMemory.h"

#if defined(PLATFORM_WDM) || defined(PLATFORM_KMDF)

void* Argus::Memory::FKrnlMemory::Malloc(POOL_FLAGS PoolFlags, SIZE_T Size, ULONG Tag) noexcept
{
	if (Size == 0 || Tag == 0 || (PoolFlags & POOL_FLAG_RAISE_ON_FAILURE) != 0)
	{
		return nullptr;
	}

	return ExAllocatePool2(PoolFlags, Size, Tag);
}

void Argus::Memory::FKrnlMemory::Free(void* Allocation) noexcept
{
	if (Allocation != nullptr)
	{
		ExFreePool(Allocation);
	}
}

#endif
