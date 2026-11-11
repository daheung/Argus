#pragma once

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/SelectSIntType.h"

#include <ctype.h>

template <typename CharType, const uint32 Size>
struct TCharBase
{
	static constexpr CharType LineFeed			 = (CharType)0xa;
	static constexpr CharType VerticalTab		 = (CharType)0xb;
	static constexpr CharType FormFeed			 = (CharType)0xc;
	static constexpr CharType CarriageReturn	 = (CharType)0xd;
	static constexpr CharType NextLine			 = (CharType)0x85;
	static constexpr CharType LineSeparator		 = (CharType)0x2028;
	static constexpr CharType ParagraphSeparator = (CharType)0x2029;

	static bool IsLineBreak(CharType Char)
	{
		return ((uint32)Char - LineFeed) <= (uint32)(CarriageReturn - LineFeed) |
			(Char == NextLine) | (Char == LineSeparator) | (Char == ParagraphSeparator);
	}
};

template <typename CharType>
struct TCharBase<CharType, 1>
{
	static constexpr CharType LineFeed		 = (CharType)0xa;
	static constexpr CharType VerticalTab	 = (CharType)0xb;
	static constexpr CharType FormFeed		 = (CharType)0xc;
	static constexpr CharType CarriageReturn = (CharType)0xd;

	static bool IsLineBreak(CharType Char)
	{
		return ((uint32)Char - LineFeed) <= (uint32)(CarriageReturn - LineFeed);
	}
};

template <typename CharType>
struct TChar :
	public TCharBase<CharType, sizeof(CharType)>
{
	/**
	* Only converts ASCII characters, same as CRT to[w]upper() with standard C locale
	*/
	static CharType ToUpper(CharType Char)
	{
		return (CharType)(ToUnsigned(Char) - (((uint32)Char - (uint32)'a' < 26u) << 5));
	}

	/**
	* Only converts ASCII characters, same as CRT to[w]upper() with standard C locale
	*/
	static CharType ToLower(CharType Char)
	{
		return (CharType)(ToUnsigned(Char) + (((uint32)Char - (uint32)'A' < 26u) << 5));
	}

	static bool IsUpper(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsLower(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsAlpha(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsGraph(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsPrint(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsPunct(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsAlnum(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsDigit(CharType Char)		{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsHexDigit(CharType Char)	{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsWhitespace(CharType Char)	{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }
	static bool IsControl(CharType Char)	{ UNREFERENCED_PARAMETER(Char); static_assert(sizeof(CharType) == 0, "Not supported."); return false; }

	static constexpr FORCEINLINE uint32 ToUnsigned(CharType Char)
	{
		return (typename TSelectUIntType<sizeof(CharType)>::Type)Char;
	}
};

template<> inline bool TChar<ANSICHAR>::IsUpper(ANSICHAR Char)		{ return isupper ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsLower(ANSICHAR Char)		{ return islower ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsAlpha(ANSICHAR Char)		{ return isalpha ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsGraph(ANSICHAR Char)		{ return isgraph ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsPrint(ANSICHAR Char)		{ return isprint ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsPunct(ANSICHAR Char)		{ return ispunct ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsAlnum(ANSICHAR Char)		{ return isalnum ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsDigit(ANSICHAR Char)		{ return isdigit ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsHexDigit(ANSICHAR Char)	{ return isxdigit((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsWhitespace(ANSICHAR Char) { return isspace ((unsigned char)Char) != 0; }
template<> inline bool TChar<ANSICHAR>::IsControl(ANSICHAR Char)	{ return iscntrl ((unsigned char)Char) != 0; }

template<> inline bool TChar<WIDECHAR>::IsUpper(WIDECHAR Char)		{ return iswupper (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsLower(WIDECHAR Char)		{ return iswlower (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsAlpha(WIDECHAR Char)		{ return iswalpha (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsGraph(WIDECHAR Char)		{ return iswgraph (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsPrint(WIDECHAR Char)		{ return iswprint (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsPunct(WIDECHAR Char)		{ return iswpunct (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsAlnum(WIDECHAR Char)		{ return iswalnum (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsDigit(WIDECHAR Char)		{ return iswdigit (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsHexDigit(WIDECHAR Char)	{ return iswxdigit(Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsWhitespace(WIDECHAR Char) { return iswspace (Char) != 0; }
template<> inline bool TChar<WIDECHAR>::IsControl(WIDECHAR Char)	{ return iswcntrl (Char) != 0; }
