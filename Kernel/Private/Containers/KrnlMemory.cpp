#include "Kernel/Public/Containers/KrnlMemory.h"

#if defined(PLATFORM_WDM) || defined(PLATFORM_KMDF)

void* Argus::Memory::FKrnlMemory::Malloc(POOL_FLAGS PoolFlags, SIZE_T Size, ULONG Tag) noexcept
{
	return MallocWithAlign(PoolFlags, Size, MEMORY_ALLOCATION_ALIGNMENT, Tag);
}

void* Argus::Memory::FKrnlMemory::MallocWithAlign(POOL_FLAGS PoolFlags, SIZE_T Size, SIZE_T Alignment, ULONG Tag) noexcept
{
	const POOL_FLAGS PoolKind = PoolFlags & (POOL_FLAG_PAGED | POOL_FLAG_NON_PAGED);
	if (Size == 0 || Tag == 0 || Alignment == 0 || (Alignment & (Alignment - 1)) != 0 ||
		(PoolFlags & POOL_FLAG_RAISE_ON_FAILURE) != 0 ||
		(PoolKind != POOL_FLAG_PAGED && PoolKind != POOL_FLAG_NON_PAGED))
	{
		return nullptr;
	}

	const SIZE_T HeaderSize = sizeof(void*);
	const SIZE_T Padding = Alignment - 1;
	const SIZE_T MaxSize = static_cast<SIZE_T>(-1);
	if (Padding > MaxSize - HeaderSize || Size > MaxSize - HeaderSize - Padding)
	{
		return nullptr;
	}

	void* PoolPointer = ExAllocatePool2(PoolFlags, Size + HeaderSize + Padding, Tag);
	if (PoolPointer == nullptr)
	{
		return nullptr;
	}

	const ULONG_PTR FirstAddress = reinterpret_cast<ULONG_PTR>(PoolPointer) + HeaderSize;
	const ULONG_PTR AlignedAddress = (FirstAddress + Padding) & ~static_cast<ULONG_PTR>(Padding);
	void* Allocation = reinterpret_cast<void*>(AlignedAddress);
	// Free recovers the original pool pointer from the bytes before Allocation.
	RtlCopyMemory(static_cast<UCHAR*>(Allocation) - HeaderSize, &PoolPointer, HeaderSize);
	return Allocation;
}

void Argus::Memory::FKrnlMemory::Free(void* Allocation) noexcept
{
	if (Allocation != nullptr)
	{
		void* PoolPointer = nullptr;
		RtlCopyMemory(&PoolPointer, static_cast<UCHAR*>(Allocation) - sizeof(PoolPointer), sizeof(PoolPointer));
		ExFreePool(PoolPointer);
	}
}

#endif
