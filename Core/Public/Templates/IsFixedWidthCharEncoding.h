#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/IsSame.h"
#include "Core/Public/Templates/IsChar.h"
#include "Core/Public/Templates/AndOrNot.h"
#include "Core/Public/Templates/RemoveConstVolatile.h"

namespace Core::Private::TypeTraits
{
	template <typename Encoding>
	constexpr bool IsFixedWidthCharEncoding()
	{
		return TOr<
			TIsSame<Encoding, ANSICHAR>,
			TIsSame<Encoding, WIDECHAR>,
			TIsSame<Encoding, UTF32CHAR>
		>::Value;
	}
}

template <typename Encoding>
struct TIsFixedWidthCharEncoding
{
	static_assert(TIsChar<Encoding>::Value, "Encoding is not a character encoding type.");

	static constexpr bool Value = Core::Private::TypeTraits::IsFixedWidthCharEncoding<typename TRemoveConstVolatile<Encoding>::Type>();
};