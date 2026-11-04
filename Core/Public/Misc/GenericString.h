#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/CString.h"
#include "Core/Public/Math/NumericLimits.h"
#include "Core/Public/Templates/IsSame.h"
#include "Core/Public/Templates/IsFixedWidthCharEncoding.h"
#include "Core/Public/Templates/IsCharEncodingSimplyConvertibleTo.h"
#include "Core/Public/Memory/Memory.h"

namespace Core::Private::String
{
	/**
	 * @brief Counts UTF8CHAR elements needed to convert a null-terminated WIDECHAR string.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Null-terminated input. Its terminator is not counted.
	 * @return Required UTF8CHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const UTF8CHAR* Dest, const WIDECHAR* Source);

	/**
	 * @brief Counts UTF8CHAR elements needed to convert a WIDECHAR range.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Start of the input range.
	 * @param SrcLen Number of WIDECHAR elements to read, including a terminator if present.
	 * @return Required UTF8CHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const UTF8CHAR* Dest, const WIDECHAR* Source, int32 SrcLen);

	/**
	 * @brief Counts UTF8CHAR elements needed to convert a null-terminated UTF32CHAR string.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Null-terminated input. Its terminator is not counted.
	 * @return Required UTF8CHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const UTF8CHAR* Dest, const UTF32CHAR* Source);

	/**
	 * @brief Counts UTF8CHAR elements needed to convert a UTF32CHAR range.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Start of the input range.
	 * @param SrcLen Number of UTF32CHAR elements to read, including a terminator if present.
	 * @return Required UTF8CHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const UTF8CHAR* Dest, const UTF32CHAR* Source, int32 SrcLen);

	/**
	 * @brief Counts ANSICHAR elements needed to convert a null-terminated UTF8CHAR string.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Null-terminated input. Its terminator is not counted.
	 * @return Required ANSICHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const ANSICHAR* Dest, const UTF8CHAR* Source);

	/**
	 * @brief Counts ANSICHAR elements needed to convert a UTF8CHAR range.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Start of the input range.
	 * @param SrcLen Number of UTF8CHAR elements to read, including a terminator if present.
	 * @return Required ANSICHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const ANSICHAR* Dest, const UTF8CHAR* Source, int32 SrcLen);

	/**
	 * @brief Counts WIDECHAR elements needed to convert a null-terminated UTF8CHAR string.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Null-terminated input. Its terminator is not counted.
	 * @return Required WIDECHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const WIDECHAR* Dest, const UTF8CHAR* Source);

	/**
	 * @brief Counts WIDECHAR elements needed to convert a UTF8CHAR range.
	 * @param Dest Unused destination type tag; may be nullptr.
	 * @param Source Start of the input range.
	 * @param SrcLen Number of UTF8CHAR elements to read, including a terminator if present.
	 * @return Required WIDECHAR elements, or -1 on conversion failure.
	 */
	int32 GetConvertedLength(const WIDECHAR* Dest, const UTF8CHAR* Source, int32 SrcLen);

	/**
	 * @brief Converts a null-terminated WIDECHAR string to UTF8CHAR elements.
	 * @param Dest Output buffer; the source terminator is not copied.
	 * @param DestLen Output capacity in UTF8CHAR elements.
	 * @param Src Null-terminated input string.
	 * @return Pointer to the next unwritten UTF8CHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const WIDECHAR* Src);

	/**
	 * @brief Converts a WIDECHAR range to UTF8CHAR elements.
	 * @param Dest Output buffer.
	 * @param DestLen Output capacity in UTF8CHAR elements.
	 * @param Src Start of the input range.
	 * @param SrcLen Number of WIDECHAR elements to convert, including a terminator if present.
	 * @return Pointer to the next unwritten UTF8CHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const WIDECHAR* Src, int32 SrcLen);

	/**
	 * @brief Converts a null-terminated UTF32CHAR string to UTF8CHAR elements.
	 * @param Dest Output buffer; the source terminator is not copied.
	 * @param DestLen Output capacity in UTF8CHAR elements.
	 * @param Src Null-terminated input string.
	 * @return Pointer to the next unwritten UTF8CHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const UTF32CHAR* Src);

	/**
	 * @brief Converts a UTF32CHAR range to UTF8CHAR elements.
	 * @param Dest Output buffer.
	 * @param DestLen Output capacity in UTF8CHAR elements.
	 * @param Src Start of the input range.
	 * @param SrcLen Number of UTF32CHAR elements to convert, including a terminator if present.
	 * @return Pointer to the next unwritten UTF8CHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const UTF32CHAR* Src, int32 SrcLen);

	/**
	 * @brief Converts a null-terminated UTF8CHAR string to ANSICHAR elements.
	 * @param Dest Output buffer; the source terminator is not copied.
	 * @param DestLen Output capacity in ANSICHAR elements.
	 * @param Src Null-terminated input string.
	 * @return Pointer to the next unwritten ANSICHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	ANSICHAR* Convert(ANSICHAR* Dest, int32 DestLen, const UTF8CHAR* Src);

	/**
	 * @brief Converts a UTF8CHAR range to ANSICHAR elements.
	 * @param Dest Output buffer.
	 * @param DestLen Output capacity in ANSICHAR elements.
	 * @param Src Start of the input range.
	 * @param SrcLen Number of UTF8CHAR elements to convert, including a terminator if present.
	 * @return Pointer to the next unwritten ANSICHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	ANSICHAR* Convert(ANSICHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen);

	/**
	 * @brief Converts a null-terminated UTF8CHAR string to WIDECHAR elements.
	 * @param Dest Output buffer; the source terminator is not copied.
	 * @param DestLen Output capacity in WIDECHAR elements.
	 * @param Src Null-terminated input string.
	 * @return Pointer to the next unwritten WIDECHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	WIDECHAR* Convert(WIDECHAR* Dest, int32 DestLen, const UTF8CHAR* Src);

	/**
	 * @brief Converts a UTF8CHAR range to WIDECHAR elements.
	 * @param Dest Output buffer.
	 * @param DestLen Output capacity in WIDECHAR elements.
	 * @param Src Start of the input range.
	 * @param SrcLen Number of UTF8CHAR elements to convert, including a terminator if present.
	 * @return Pointer to the next unwritten WIDECHAR element on success, or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; the current implementation may write a prefix.
	 */
	WIDECHAR* Convert(WIDECHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen);
}

#define HIGH_SURROGATE_START_CODEPOINT    ((uint16)0xD800)
#define HIGH_SURROGATE_END_CODEPOINT      ((uint16)0xDBFF)
#define LOW_SURROGATE_START_CODEPOINT     ((uint16)0xDC00)
#define LOW_SURROGATE_END_CODEPOINT       ((uint16)0xDFFF)
#define ENCODED_SURROGATE_START_CODEPOINT ((uint32)0x10000)
#define ENCODED_SURROGATE_END_CODEPOINT   ((uint32)0x10FFFF)

#define UNICODE_BOGUS_CHAR_CODEPOINT '?'

static_assert(
	sizeof(UNICODE_BOGUS_CHAR_CODEPOINT) <= sizeof(ANSICHAR) && 
	(UNICODE_BOGUS_CHAR_CODEPOINT) >= 32 && 
	(UNICODE_BOGUS_CHAR_CODEPOINT) <= 127, 
	"The Unicode Bogus character point is expected to fit in a single ANSICHAR here"
);

struct FGenericString
{
	template <typename DestEncoding, typename SourceEncoding>
	static constexpr bool CanConvertCodepoint(SourceEncoding Codepoint)
	{
		static_assert(TIsChar<SourceEncoding>::Value, "Source encoding is not a char type");
		static_assert(TIsChar<DestEncoding	>::Value, "Destination encoding is not a char type");

		static_assert(TIsFixedWidthCharEncoding<SourceEncoding>::Value, "Source encoding is not fixed-width");

		if constexpr (TIsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>::Value)
		{
			return true;
		}
		else if constexpr (!TIsFixedWidthCharEncoding<DestEncoding>::Value)
		{
			return true;
		}
		else if constexpr (TIsSame<DestEncoding, ANSICHAR>::Value)
		{
			return (uint32)Codepoint <= 0x7F;
		}
		else
		{
			static_assert(sizeof(SourceEncoding) == 4 && sizeof(DestEncoding) == 2, "Unimplemented conversion");
			return (uint32)Codepoint <= 0xFFFF;
		}
	}

	/**
	 * @brief Converts a source range into the destination encoding.
	 * @param Dest Output buffer.
	 * @param DestSize Output capacity in DestEncoding elements.
	 * @param Src Start of the input range.
	 * @param SrcSize Number of SourceEncoding elements to convert.
	 * @return Pointer to the next unwritten DestEncoding element on success,
	 *         or nullptr on failure.
	 * @todo On failure, leave Dest unchanged; some conversion paths currently write a prefix.
	 */
	template <typename SourceEncoding, typename DestEncoding>
	static FORCEINLINE DestEncoding* Convert(DestEncoding* Dest, int32 DestSize, const SourceEncoding* Src, int32 SrcSize)
	{
		if constexpr (TIsCharEncodingCompatibleWith<SourceEncoding, DestEncoding>::Value)
		{
			if (DestSize < SrcSize)
			{
				return nullptr;
			}

			return (DestEncoding*)FMemory::Memcpy(Dest, Src, SrcSize * sizeof(SourceEncoding)) + SrcSize;
		}
		else if constexpr (TIsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>::Value)
		{
			const int32 Size = DestSize <= SrcSize ? DestSize : SrcSize;
			for (int I = 0; I < Size; ++I)
			{
				SourceEncoding SrcCh = Src[I];
				Dest[I] = (DestEncoding)SrcCh;
			}

			return DestSize < SrcSize ? nullptr : Dest + Size;
		}
		else if constexpr (TAnd<TIsFixedWidthCharEncoding<SourceEncoding>, TIsFixedWidthCharEncoding<DestEncoding>>::Value)
		{
			const int32 Size = DestSize <= SrcSize ? DestSize : SrcSize;
			bool bInvalidChars = false;
			for (int I = 0; I < Size; ++I)
			{
				SourceEncoding SrcCh = Src[I];
				Dest[I] = SrcCh;
				bInvalidChars |= !CanConvertCodepoint<DestEncoding>(SrcCh);
			}

			if (bInvalidChars)
			{
				for (int I = 0; I < Size; ++I)
				{
					if (!CanConvertCodepoint<DestEncoding>(Src[I]))
					{
						Dest[I] = UNICODE_BOGUS_CHAR_CODEPOINT;
					}
				}

				//LogBugsChars<DestEncoding>(Src, Size);
			}

			return DestSize < SrcSize ? nullptr : Dest + Size;
		}
		else
		{
			return Core::Private::String::Convert(Dest, DestSize, Src, SrcSize);
		}
	}

	/**
	 * @brief Returns the DestEncoding element count needed for a null-terminated source string.
	 *
	 * The count includes the null terminator. It is a count of destination
	 * elements, not bytes or Unicode code points. For byte-compatible pairs
	 * such as UTF16CHAR -> WIDECHAR, the source and destination element counts
	 * are equal, including the terminator.
	 *
	 * @param Src Null-terminated source string.
	 * @return Required DestEncoding elements, including the null terminator,
	 *         or -1 if the count exceeds int32.
	 */
	template <typename DestEncoding, typename SourceEncoding>
	static FORCEINLINE int32 ConvertedLength(const SourceEncoding* Src)
	{
		if constexpr (
			TOr<
				TIsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>,
				TAnd<
					TIsFixedWidthCharEncoding<SourceEncoding>,
					TIsFixedWidthCharEncoding<DestEncoding>
				>
			>::Value
		) {
			const size_t SourceLength = TCString<SourceEncoding>::Strlen(Src);
			if (SourceLength >= static_cast<size_t>(TNumericLimits<int32>::Max()))
			{
				return INDEX_NONE;
			}

			return static_cast<int32>(SourceLength + 1);
		}
		else
		{
			const size_t Length = Core::Private::String::GetConvertedLength((DestEncoding*)nullptr, Src);
			if (Length == INDEX_NONE)
			{
				return INDEX_NONE;
			}

			return Length + 1;
		}
	}

	/**
	 * @brief Returns the DestEncoding element count for the [Src, Src + SrcLen) range.
	 *
	 * SrcLen counts source elements, not bytes or Unicode code points. For a
	 * one-to-one code-unit copy such as UTF16CHAR -> WIDECHAR, the result is
	 * SrcLen. A null terminator is counted only if it is part of the range;
	 * this overload does not add one.
	 *
	 * @param Src Start of the source range.
	 * @param SrcLen Number of SourceEncoding elements in the range.
	 * @return Required DestEncoding elements.
	 */
	template <typename DestEncoding, typename SourceEncoding>
	static FORCEINLINE int32 ConvertedLength(const SourceEncoding* Src, int32 SrcLen)
	{
		if constexpr (
			TOr<
				TIsCharEncodingSimplyConvertibleTo<SourceEncoding, DestEncoding>,
				TAnd<
					TIsFixedWidthCharEncoding<SourceEncoding>,
					TIsFixedWidthCharEncoding<DestEncoding>
				>
			>::Value
		) 
		{
			return SrcLen;
		}
		else
		{
			return Core::Private::String::GetConvertedLength((DestEncoding*)nullptr, Src, SrcLen);
		}
	}
};
