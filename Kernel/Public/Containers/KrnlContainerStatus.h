#pragma once

#include "Core/Public/Misc/Types.h"

enum class EContainerStatus : uint8
{
	Success,
	InvalidParameter,
	InvalidState,
	SizeOverflow,
	AllocationFailed,
	CheckFailed
};

#define ContainerSuccess(Status) \
	(Status == EContainerStatus::Success)

#define ReturnIfFalse(expr)				\
    do									\
    {									\
        const bool bSuccess = (expr);	\
        if (!bSuccess)                  \
        {								\
            return false;				\
        }								\
    } while (false)