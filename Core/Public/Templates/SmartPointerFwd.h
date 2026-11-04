#pragma once

#include "Core/Public/Misc/Types.h"

typedef int32 RefCounter;

enum class ESPMode
{
	NonThreadSafe = 0,
	ThreadSafe = 1
};

template <typename ObjectType, ESPMode InMode = ESPMode::ThreadSafe> class TSharedPtr;
template <typename ObjectType, ESPMode InMode = ESPMode::ThreadSafe> class TWeakPtr;
template <typename ObjectType, ESPMode InMode = ESPMode::ThreadSafe> class TUniquePtr;