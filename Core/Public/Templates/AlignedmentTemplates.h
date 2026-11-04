#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Templates/IsIntegral.h"
#include "Core/Public/Templates/IsPointer.h"

/**
 * Checks if a pointer is aligned to the specified alignment.
 *
 * @param  Val        The value to align.
 * @param  Alignment  The alignment value, must be a power of two.
 *
 * @return true if the pointer is aligned to the specified alignment, false otherwise.
 */
template <typename T>
FORCEINLINE constexpr bool IsAligned(T Val, int64 Alignment)
{
	static_assert(TOr<TIsIntegral<T>, TIsPointer<T>>::Value, "IsAligned expects an integer or pointer type");

	return !((uint64)Val & (Alignment - 1));
}