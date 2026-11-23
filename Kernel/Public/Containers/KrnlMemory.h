#pragma once
#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"
#include "Kernel/Public/Containers/KrnlContainerType.h"
#include "Kernel/Public/Misc/KernelDefines.h"

namespace Argus::Memory
{
	struct FKrnlMemory
	{
		// PoolFlags must select paged or nonpaged pool. Release the result with Free.
		NODISCARD static void* Malloc(POOL_FLAGS PoolFlags, SIZE_T Size, ULONG Tag) noexcept;
		NODISCARD static void* MallocWithAlign(POOL_FLAGS PoolFlags, SIZE_T Size, SIZE_T Alignment, ULONG Tag) noexcept;

		template <EPoolType PoolType>
		NODISCARD static void* Malloc(SIZE_T Size, ULONG Tag) noexcept
		{
			static_assert(PoolType == EPoolType::Paged || PoolType == EPoolType::NonPaged, "Unsupported kernel pool type");

			constexpr POOL_FLAGS Flags = PoolType == EPoolType::Paged
				? POOL_FLAG_PAGED : POOL_FLAG_NON_PAGED;
			return Malloc(Flags, Size, Tag);
		}

		template <EPoolType PoolType>
		NODISCARD static void* MallocAlignment(SIZE_T Size, SIZE_T Alignment, ULONG Tag) noexcept
		{
			static_assert(PoolType == EPoolType::Paged || PoolType == EPoolType::NonPaged, "Unsupported kernel pool type");

			constexpr POOL_FLAGS Flags = PoolType == EPoolType::Paged
				? POOL_FLAG_PAGED : POOL_FLAG_NON_PAGED;
			return MallocWithAlign(Flags, Size, Alignment, Tag);
		}

		template <EPoolType PoolType, typename T>
		NODISCARD static T* MallocAlignment(ULONG Tag) noexcept
		{
			return static_cast<T*>(MallocAlignment<PoolType>(sizeof(T), alignof(T), Tag));
		}

		// Allocation must come from FKrnlMemory::Malloc or MallocWithAlign.
		static void Free(void* Allocation) noexcept;
	};
}

using FKrnlMemory = Argus::Memory::FKrnlMemory;
