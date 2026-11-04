#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/IsChar.h"
#include "Core/Public/Templates/IsCharEncodingCompatibleWith.h"
#include "Core/Public/Templates/IsFixedWidthCharEncoding.h"

namespace Core::Private::TypeTraits
{
	template <typename SrcEncoding, typename DestEncoding>
	constexpr bool IsCharEncodingSimplyConvertibleToImpl()
	{
		if constexpr (TIsCharEncodingCompatibleWith<SrcEncoding, DestEncoding>::Value)
		{
			return true;
		}
		else if constexpr (TIsFixedWidthCharEncoding<SrcEncoding>::Value && sizeof(DestEncoding) >= sizeof(SrcEncoding))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}

/**
 * @brief Identifies pairs that use byte copying or one-to-one element casting.
 *
 * Returns true when the pair is byte-copy compatible, or when SrcEncoding is
 * classified as fixed-width and sizeof(DestEncoding) >= sizeof(SrcEncoding).
 * Compatible pairs are copied; other simple pairs cast each source element
 * to one destination element. Both paths preserve the element count.
 *
 * This trait does not cover every one-to-one conversion: narrowing between
 * two fixed-width types is handled separately. The size check does not
 * validate encoded data or mean that one element is one Unicode code point;
 * a UTF-16 surrogate pair still occupies two elements.
 */
template <typename SrcEncoding, typename DestEncoding>
struct TIsCharEncodingSimplyConvertibleTo
{
	static_assert(TIsChar<SrcEncoding >::Value, "SrcEncoding is not a character encoding type");
	static_assert(TIsChar<DestEncoding>::Value, "DestEncoding is not a character encoding type");

	static constexpr bool Value = Core::Private::TypeTraits::IsCharEncodingSimplyConvertibleToImpl<SrcEncoding, DestEncoding>();
};
