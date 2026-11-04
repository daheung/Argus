#pragma once

#include "Core/Public/Misc/Types.h"

#include "Core/Public/Templates/IsSame.h"
#include "Core/Public/Templates/AndOrNot.h"
#include "Core/Public/Templates/RemoveConstVolatile.h"

namespace Core::Private::TypeTraits
{
	template <typename SrcEncoding, typename DestEncoding>
	constexpr bool IsCharEncodingCompatibleWithImpl()
	{
		return TOr<
			TIsSame<DestEncoding, SrcEncoding>,
			TAnd<TIsSame<SrcEncoding, ANSICHAR >, TIsSame<DestEncoding, UTF8CHAR >>,
			TAnd<TIsSame<SrcEncoding, WIDECHAR >, TIsSame<DestEncoding, UTF16CHAR>>,
			TAnd<TIsSame<SrcEncoding, UTF16CHAR>, TIsSame<DestEncoding, WIDECHAR >>
		>::Value;

		/**
		 *	if constexpr (TIsSame<SrcEncoding, DestEncoding>)
		 *	{
		 *		return true;
		 *	}
		 *	else if constexpr (TIsSame<SrcEncoding, ANSICHAR> && TIsSame<DestEncoding, UTF8CHAR>)
		 *	{
		 *		return true;
		 *	}
		 *	else
		 *	{
		 *		return false;
		 *	}
		 */
	}
}

/**
 * @brief Tests whether SrcEncoding elements may be copied byte-for-byte into DestEncoding elements.
 *
 * A compatible pair has equal-sized elements, so copying
 * SrcSize * sizeof(SrcEncoding) bytes preserves the bit patterns of SrcSize
 * elements without decoding or converting them. When WIDECHAR is a 16-bit
 * UTF-16 code-unit type, copying between WIDECHAR and UTF16CHAR in either
 * direction preserves the UTF-16 code-unit sequence.
 *
 * This does not mean that each element represents one Unicode code point:
 * a UTF-16 surrogate pair occupies two elements. The trait does not decode
 * or validate the contents of the source string.
 *
 * The source encoding need not be fixed-width. WIDECHAR <-> UTF16CHAR is
 * compatible in both directions. ANSICHAR -> UTF8CHAR is compatible only in
 * that direction; UTF8CHAR -> ANSICHAR is not declared compatible.
 */
template <typename SrcEncoding, typename DestEncoding>
struct TIsCharEncodingCompatibleWith
{
	static_assert(sizeof(UTF16CHAR) == sizeof(WIDECHAR), "WIDECHAR and UTF16CHAR must have the same size for byte-copy compatibility.");
	static_assert(TAnd<TIsChar<SrcEncoding>, TIsChar<DestEncoding>>::Value, "Both types must be character encoding types.");

	static constexpr bool Value = Core::Private::TypeTraits::IsCharEncodingCompatibleWithImpl<
		typename TRemoveConstVolatile<SrcEncoding>::Type, typename TRemoveConstVolatile<DestEncoding>::Type>();
};

template <typename SrcEncoding>
using TIsCharEncodingCompatibleWithTCHAR = TIsCharEncodingCompatibleWith<SrcEncoding, wchar_t>;
