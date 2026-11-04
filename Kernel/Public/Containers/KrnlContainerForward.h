#pragma once

#include "Core/Public/Misc/Types.h"
#include "Kernel/Public/Containers/KrnlPoolType.h"

template <typename ElementType, typename SizeType, EPoolType PoolType>
class TKrnlElLinearAllocator;

template <EPoolType PoolType, typename ElementType, typename AllocatorType>
class TKrnlArray;

template <EPoolType PoolType, typename ElementType, typename AllocatorType>
class TKrnlSpscCircularQueue;

template <EPoolType PoolType>
class FKrnlString;

template <EPoolType PoolType, typename ElementType, typename AllocatorType = TKrnlElLinearAllocator<ElementType, int32, PoolType>>
using TArray = TKrnlArray<PoolType, ElementType, AllocatorType>;

template <EPoolType PoolType, typename ElementType, typename AllocatorType = TKrnlElLinearAllocator<ElementType, int32, PoolType>>
using TSpscCircularQueue = TKrnlSpscCircularQueue<PoolType, ElementType, AllocatorType>;

template <EPoolType PoolType = EPoolType::Paged>
using FString = FKrnlString<PoolType>;
