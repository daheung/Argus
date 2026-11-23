#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Public/Containers/KrnlContainerType.h"

typedef int32 RefCounter;

enum class ESPMode
{
	NonThreadSafe = 0,
	ThreadSafe = 1
};

template <typename ObjectType, EPoolType PoolType = EPoolType::NonPaged, ESPMode InMode = ESPMode::ThreadSafe> class TKrnlSharedPtr;
template <typename ObjectType, EPoolType PoolType = EPoolType::NonPaged, ESPMode InMode = ESPMode::ThreadSafe> class TKrnlWeakPtr;

namespace SharedPointerInternals
{
	template <typename ObjectType> struct FDefaultDeleter;
}

template <
	typename ObjectType, 
	EPoolType PoolType = EPoolType::NonPaged, 
	ESPMode InMode = ESPMode::ThreadSafe, 
	typename DeleterType = SharedPointerInternals::FDefaultDeleter<ObjectType>
>
class TKrnlUniquePtr;

template <
	typename ObjectType,
	EPoolType PoolType = EPoolType::NonPaged,
	ESPMode InMode = ESPMode::ThreadSafe,
	typename DeleterType = SharedPointerInternals::FDefaultDeleter<ObjectType>
>
using TUniquePtr = TKrnlUniquePtr<ObjectType, PoolType, InMode, DeleterType>;