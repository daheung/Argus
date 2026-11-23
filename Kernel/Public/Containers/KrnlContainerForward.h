#pragma once

#include "Core/Public/Misc/Types.h"
#include "Kernel/Public/Containers/KrnlContainerType.h"

template <typename ElementType, typename SizeType, EPoolType PoolType>
class TKrnlElLinearAllocator;

template <EPoolType PoolType, typename ElementType, typename AllocatorType>
class TKrnlArray;

template <EPoolType PoolType, typename ElementType, typename AllocatorType>
class TKrnlSpscCircularQueue;

template <EPoolType PoolType>
class FKrnlString;

template <EContainerLockType ContainerLockType>
class TKrnlContainerLock;

template <size_t Capacity>
class FKrnlName;

template <EPoolType PoolType, typename ElementType, typename AllocatorType = TKrnlElLinearAllocator<ElementType, int32, PoolType>>
using TArray = TKrnlArray<PoolType, ElementType, AllocatorType>;

template <typename ElementType, typename AllocatorType = TKrnlElLinearAllocator<ElementType, int32, EPoolType::Paged>>
using TPageArray = TKrnlArray<EPoolType::Paged, ElementType, AllocatorType>;

template <typename ElementType, typename AllocatorType = TKrnlElLinearAllocator<ElementType, int32, EPoolType::NonPaged>>
using TNonPageArray = TKrnlArray<EPoolType::NonPaged, ElementType, AllocatorType>;

using TPageArrayLock = TKrnlContainerLock<EContainerLockType::Wait>;

using TNonPageArrayLock = TKrnlContainerLock<EContainerLockType::Spinlock>;

template <EPoolType PoolType, typename ElementType, typename AllocatorType = TKrnlElLinearAllocator<ElementType, int32, PoolType>>
using TSpscCircularQueue = TKrnlSpscCircularQueue<PoolType, ElementType, AllocatorType>;

template <EPoolType PoolType = EPoolType::Paged>
using FString = FKrnlString<PoolType>;

using FPageString = FKrnlString<EPoolType::Paged>;

template <size_t Capacity>
using FName = FKrnlName<Capacity>;