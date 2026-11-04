#pragma once

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Assertion.h"

/**
 * @brief Aligns the specified value upward to the nearest multiple of the given alignment.
 *
 * The alignment must be a non-zero power of two.
 * If the value is already aligned, it is returned unchanged.
 *
 * @param Value The value to align.
 * @param Alignment The alignment boundary. Must be a non-zero power of two.
 *
 * @return The smallest aligned value greater than or equal to @p Value.
 */
NODISCARD static FORCEINLINE size_t AlignUp(size_t Value, size_t Alignment)
{
    CORE_CHECKF(Alignment != 0, "Alignment must be greater than 0.");
    CORE_CHECKF((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of two.");
    return (Value + Alignment - 1) & ~(Alignment - 1);
}