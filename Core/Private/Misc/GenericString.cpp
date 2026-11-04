#include "Core/Public/Misc/GenericString.h"

#include "Core/Public/Math/NumericLimits.h"

#include "Core/Public/Templates/IsSame.h"
#include "Core/Public/Templates/IsIntegral.h"
#include "Core/Public/Templates/MakeUnsigned.h"
#include "Core/Public/Templates/AlignedmentTemplates.h"

namespace Core::Private::String
{
    template <typename DestType>
    struct TCountingOutputIterator
    {
        FORCEINLINE TCountingOutputIterator()
            : Counter(0)
        {
        }

        const TCountingOutputIterator& operator*() const { return *this; }
        const TCountingOutputIterator& operator++() { ++Counter; return *this; }
        const TCountingOutputIterator& operator++(int) { ++Counter; return *this; }
        const TCountingOutputIterator& operator+=(const int32 Amount) { Counter += Amount; return *this; }

        const DestType& operator=(const DestType& Val) const
        {
            return Val;
        }

        friend int32 operator-(TCountingOutputIterator Lhs, TCountingOutputIterator Rhs)
        {
            return Lhs.Counter - Rhs.Counter;
        }

        int32 GetCounter() const { return Counter; }

    private:
        int32 Counter;
    };

    FORCEINLINE bool IsValidCodepoint(const uint32 Codepoint)
    {
        if ((Codepoint >  0x10FFFF) ||						  // No Unicode codepoints above 10FFFFh, (for now!)
            (Codepoint == 0xFFFE  ) || (Codepoint == 0xFFFF)) // illegal values.
        {
            return false;
        }

        return true;
    }

    /** Is the provided Codepoint within the range of the high-or low surrogates? */
    static FORCEINLINE bool IsSurrogate(const uint32 Codepoint)
    {
        return (Codepoint & 0xFFFFF800) == 0xD800;
    }

    /** Is the provided Codepoint within the range of the high-surrogates? */
    static FORCEINLINE bool IsHighSurrogate(const uint32 Codepoint)
    {
        return (Codepoint & 0xFFFFFC00) == 0xD800;
    }

    /** Is the provided Codepoint within the range of the low-surrogates? */
    static FORCEINLINE bool IsLowSurrogate(const uint32 Codepoint)
    {
        return (Codepoint & 0xFFFFFC00) == 0xDC00;
    }

    static FORCEINLINE uint32 EncodeSurrogate(const uint16 HighSurrogate, const uint16 LowSurrogate)
    {
        return ((HighSurrogate - HIGH_SURROGATE_START_CODEPOINT) << 10) + (LowSurrogate - LOW_SURROGATE_START_CODEPOINT) + 0x10000;
    }

    static FORCEINLINE void DecodeSurrogate(const uint32 Codepoint, uint16& OutHighSurrogate, uint16& OutLowSurrogate)
    {
        const uint32 TmpCodepoint = Codepoint - 0x10000;
        OutHighSurrogate = (uint16)((TmpCodepoint >> 10) + HIGH_SURROGATE_START_CODEPOINT);
        OutLowSurrogate = (TmpCodepoint & 0x3FF) + LOW_SURROGATE_START_CODEPOINT;
    }

    /** Is the provided Codepoint outside of the range of the basic multilingual plane, but within the valid range of UTF8/16? */
    static FORCEINLINE bool IsEncodedSurrogate(const uint32 Codepoint)
    {
        return Codepoint >= ENCODED_SURROGATE_START_CODEPOINT && Codepoint <= ENCODED_SURROGATE_END_CODEPOINT;
    }

    template <typename BufferType>
    static bool WriteCodepointToBuffer(uint32 Codepoint, BufferType& OutputIterator, int32& OutputIteratorByteSizeRemaining)
    {
        // Ensure we have at least one character in size to write
        if (OutputIteratorByteSizeRemaining == 0)
        {
            return false;
        }

        if (!IsValidCodepoint(Codepoint))
        {
            Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
        }
        else if (IsSurrogate(Codepoint)) // UTF-8 Characters are not allowed to encode codepoints in the surrogate pair range
        {
            Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        // Do the encoding...
        if (Codepoint < 0x80)
        {
            *OutputIterator++ = (UTF8CHAR)Codepoint;

            OutputIteratorByteSizeRemaining -= 1;
        }
        else if (Codepoint < 0x800)
        {
            if (OutputIteratorByteSizeRemaining < 2)
            {
                return false;
            }

            *OutputIterator++ = (UTF8CHAR)((Codepoint >> 6)         | 128 | 64);
            *OutputIterator++ = (UTF8CHAR)((Codepoint       & 0x3F) | 128);

            OutputIteratorByteSizeRemaining -= 2;
        }
        else if (Codepoint < 0x10000)
        {
            if (OutputIteratorByteSizeRemaining < 3)
            {
                return false;
            }

            *OutputIterator++ = (UTF8CHAR)( (Codepoint >> 12)         | 128 | 64 | 32);
            *OutputIterator++ = (UTF8CHAR)(((Codepoint >> 6 ) & 0x3F) | 128);
            *OutputIterator++ = (UTF8CHAR)( (Codepoint        & 0x3F) | 128);

            OutputIteratorByteSizeRemaining -= 3;
        }
        else
        {
            if (OutputIteratorByteSizeRemaining < 4)
            {
                return false;
            }

            *OutputIterator++ = (UTF8CHAR)( (Codepoint >> 18)         | 128 | 64 | 32 | 16);
            *OutputIterator++ = (UTF8CHAR)(((Codepoint >> 12) & 0x3F) | 128);
            *OutputIterator++ = (UTF8CHAR)(((Codepoint >> 6 ) & 0x3F) | 128);
            *OutputIterator++ = (UTF8CHAR)( (Codepoint        & 0x3F) | 128);

            OutputIteratorByteSizeRemaining -= 4;
        }

        return true;
    }

    struct FNullTerminal
    {
        explicit FNullTerminal() = default;
    };

    template <typename Pointer>
    FORCEINLINE bool IsRangeEmpty(const Pointer& Ptr, const int32& Len)
    {
        UNREFERENCED_PARAMETER(Ptr);
        return Len <= 0;
    }

    template <typename Pointer>
    FORCEINLINE bool IsRangeEmpty(const Pointer& Ptr, FNullTerminal)
    {
        return *Ptr == '\0';
    }

    template <typename Pointer>
    FORCEINLINE void PopFront(Pointer& Ptr, int32& Len)
    {
        CORE_CHECKF(Len > 0, "Trying to pop past end end of the range.");
        ++Ptr;
        --Len;
    }

    template <typename Pointer>
    FORCEINLINE void PopFront(Pointer& Ptr, FNullTerminal)
    {
        CORE_CHECKF(*Ptr > '\0', "Trying to pop the null terminator of the string.");
        ++Ptr;
    }

    template <typename DestBufferType, typename FromType, typename SourceEndType>
    static int32 ConvertToUTF8(DestBufferType& Dest, int32 DestLen, const FromType* Source, SourceEndType SourceEnd)
    {
        //using UnsignedFromType = typename TMakeUnsigned<FromType>::Type;
        using UnsignedFromType = typename TSelectUIntType<sizeof(FromType)>::Type;

        if constexpr (sizeof(FromType) == 4) // UTF32
        {
            DestBufferType DestStartingPosition = Dest;

            for (;;)
            {
                if (IsRangeEmpty(Source, SourceEnd))
                {
                    return static_cast<int32>(Dest - DestStartingPosition);
                }

                uint32 CodePoint = (uint32)(UnsignedFromType)*Source;
                if (!WriteCodepointToBuffer(CodePoint, Dest, DestLen))
                {
                    return -1;
                }

                PopFront(Source, SourceEnd);
            }
        }
        else // UTF16
        {
            DestBufferType DestStartingPosition = Dest;

            for (;;)
            {
                if (IsRangeEmpty(Source, SourceEnd))
                {
                    return static_cast<int32>(Dest - DestStartingPosition);
                }

                uint32 CodePoint = (uint32)(UnsignedFromType)*Source;
                PopFront(Source, SourceEnd);

                if (IsHighSurrogate(CodePoint))
                {
                    if (IsRangeEmpty(Source, SourceEnd))
                    {
                        if (!WriteCodepointToBuffer(CodePoint, Dest, DestLen))
                        {
                            return -1;
                        }

                        return static_cast<int32>(Dest - DestStartingPosition);
                    }

                    uint32 NextCodePoint = (uint32)(UnsignedFromType)*Source;

                    if (IsLowSurrogate(NextCodePoint))
                    {
                        CodePoint = EncodeSurrogate((uint16)CodePoint, (uint16)NextCodePoint);
                        PopFront(Source, SourceEnd);
                    }

                }

                if (!WriteCodepointToBuffer(CodePoint, Dest, DestLen))
                {
                    return -1;
                }
            }
        }
    }

    template <typename SourceEndType>
    static bool ReadTrailingOctet(uint32& OutOctet, const UTF8CHAR*& Ptr, SourceEndType& SourceEnd)
    {
        if (IsRangeEmpty(Ptr, SourceEnd))
        {
            return false;
        }

        uint32 Octet = (uint32)(uint8)*Ptr;
        PopFront(Ptr, SourceEnd);

        if ((Octet & 192) != 128)
        {
            return false;
        }

        OutOctet = Octet;
        return true;
    }

    template <typename SourceEndType>
    FORCEINLINE uint32 CodepointFromUTF8(const UTF8CHAR*& SourceString, SourceEndType& SourceEnd)
    {
        CORE_CHECK(!IsRangeEmpty(SourceString, SourceEnd));

        uint32 Octet = (uint32)(uint8)*SourceString;
        PopFront(SourceString, SourceEnd);

        if (Octet < 128)
        {
            return Octet;
        }

        if (Octet < 192)
        {
            return UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        uint32 Octet2;
        if (!ReadTrailingOctet(Octet2, SourceString, SourceEnd))
        {
            return UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        if (Octet < 224)  // two octets
        {
            uint32 Codepoint = ((Octet - 192) << 6) | (Octet2 - 128);
            if (Codepoint < 0x80 || Codepoint > 0x7FF)
            {
                return UNICODE_BOGUS_CHAR_CODEPOINT;
            }

            // Should only reach here after parsing a legal codepoint
            return Codepoint;
        }

        uint32 Octet3;
        if (!ReadTrailingOctet(Octet3, SourceString, SourceEnd))
        {
            return UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        if (Octet < 240)  // three octets
        {
            uint32 Codepoint = ((Octet - 224) << 12) | ((Octet2 - 128) << 6) | (Octet3 - 128);

            // UTF-8 characters cannot be in the UTF-16 surrogates range.  0xFFFE and 0xFFFF are illegal, too, so we check them at the edge.
            if (Codepoint < 0x800 || Codepoint > 0xFFFD || IsSurrogate(Codepoint))
            {
                return UNICODE_BOGUS_CHAR_CODEPOINT;
            }

            // Should only reach here after parsing a legal codepoint
            return Codepoint;
        }

        uint32 Octet4;
        if (!ReadTrailingOctet(Octet4, SourceString, SourceEnd))
        {
            return UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        if (Octet < 248)  // four octets
        {
            Octet -= (128 + 64 + 32 + 16);

            uint32 Codepoint = (Octet << 18) | ((Octet2 - 128) << 12) | ((Octet3 - 128) << 6) | (Octet4 - 128);
            if (Codepoint < 0x10000 || Codepoint > 0x10FFFF)
            {
                return UNICODE_BOGUS_CHAR_CODEPOINT;
            }

            // Should only reach here after parsing a legal codepoint
            return Codepoint;
        }

        uint32 Octet5;
        if (!ReadTrailingOctet(Octet5, SourceString, SourceEnd))
        {
            return UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        // Five and six octet sequences became illegal in rfc3629.
        //  We throw the codepoint away, but parse them to make sure we move
        //  ahead the right number of bytes and don't overflow the buffer.

        if (Octet < 252)  // five octets
        {
            // Skip to end and write out a single char (we always have room for at least 1 char)
            return UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        // six octets

        uint32 Octet6;
        if (!ReadTrailingOctet(Octet6, SourceString, SourceEnd))
        {
            return UNICODE_BOGUS_CHAR_CODEPOINT;
        }

        return UNICODE_BOGUS_CHAR_CODEPOINT;
    }

    template <typename DestType, typename DestBufferType, typename SourceEndType>
    static int32 ConvertFromUTF8(DestBufferType& ConvertedBuffer, int32 DestLen, const UTF8CHAR* Source, SourceEndType SourceEnd)
    {
        DestBufferType DestStartingPosition = ConvertedBuffer;

        constexpr uint64 ExtendedCheckMask = 0x8080808080808080;
        while (!IsRangeEmpty(Source, SourceEnd))
        {
            if (DestLen == 0)
            {
                return -1;
            }

            // Read eight bytes at once only when SourceEnd provides a length;
            // a null-terminated source may have fewer than eight readable bytes.
            if constexpr (TIsIntegral<SourceEndType>::Value)
            {
                if (IsAligned(Source, 8))
                {
                    while (SourceEnd >= 8 && DestLen >= 8)
                    {
                        if ((*(const uint64*)Source) & ExtendedCheckMask)
                        {
                            break;
                        }

                        for (int32 Index = 0; Index < 8; ++Index)
                        {
                            *(ConvertedBuffer++) = (DestType)(uint8)*(Source++);
                        }

                        SourceEnd -= 8;
                        DestLen -= 8;
                    }
                }
            }

            while (!IsRangeEmpty(Source, SourceEnd) && DestLen > 0)
            {
                uint32 Codepoint = CodepointFromUTF8(Source, SourceEnd);

                if constexpr (sizeof(DestType) != 4)
                {
                    if (IsEncodedSurrogate(Codepoint))
                    {
                        if (DestLen >= 2)
                        {
                            uint16 HighSurrogate = 0;
                            uint16 LowSurrogate = 0;
                            DecodeSurrogate(Codepoint, HighSurrogate, LowSurrogate);

                            *(ConvertedBuffer++) = (DestType)HighSurrogate;
                            *(ConvertedBuffer++) = (DestType)LowSurrogate;
                            DestLen -= 2;
                            continue;
                        }

                        Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
                    }
                    else if (Codepoint > ENCODED_SURROGATE_END_CODEPOINT)
                    {
                        Codepoint = (uint32)UNICODE_BOGUS_CHAR_CODEPOINT;
                    }
                }

                *(ConvertedBuffer++) = (DestType)Codepoint;
                --DestLen;

                if constexpr (TIsIntegral<SourceEndType>::Value)
                {
                    if (Codepoint < 128 && IsAligned(Source, 8))
                    {
                        break;
                    }
                }
            }
        }

        return static_cast<int32>(ConvertedBuffer - DestStartingPosition);
    }

    int32 GetConvertedLength(const UTF8CHAR*, const WIDECHAR* Source)
    {
        TCountingOutputIterator<UTF8CHAR> Dest;
        int32 Result = ConvertToUTF8(Dest, MAX_int32, Source, FNullTerminal{});
        return Result;
    }
    int32 GetConvertedLength(const UTF8CHAR*, const WIDECHAR* Source, int32 SourceLen)
    {
        TCountingOutputIterator<UTF8CHAR> Dest;
        int32 Result = ConvertToUTF8(Dest, MAX_int32, Source, SourceLen);
        return Result;
    }
    int32 GetConvertedLength(const UTF8CHAR*, const UTF32CHAR* Source)
    {
        TCountingOutputIterator<UTF8CHAR> Dest;
        int32 Result = ConvertToUTF8(Dest, MAX_int32, Source, FNullTerminal{});
        return Result;
    }
    int32 GetConvertedLength(const UTF8CHAR*, const UTF32CHAR* Source, int32 SourceLen)
    {
        TCountingOutputIterator<UTF8CHAR> Dest;
        int32 Result = ConvertToUTF8(Dest, MAX_int32, Source, SourceLen);
        return Result;
    }
    int32 GetConvertedLength(const ANSICHAR*, const UTF8CHAR* Source)
    {
        TCountingOutputIterator<ANSICHAR> Dest;
        int32 Result = ConvertFromUTF8<ANSICHAR>(Dest, MAX_int32, Source, FNullTerminal{});
        return Result;
    }
    int32 GetConvertedLength(const ANSICHAR*, const UTF8CHAR* Source, int32 SourceLen)
    {
        TCountingOutputIterator<ANSICHAR> Dest;
        int32 Result = ConvertFromUTF8<ANSICHAR>(Dest, MAX_int32, Source, SourceLen);
        return Result;
    }
    int32 GetConvertedLength(const WIDECHAR*, const UTF8CHAR* Source)
    {
        TCountingOutputIterator<WIDECHAR> Dest;
        int32 Result = ConvertFromUTF8<WIDECHAR>(Dest, MAX_int32, Source, FNullTerminal{});
        return Result;
    }
    int32 GetConvertedLength(const WIDECHAR*, const UTF8CHAR* Source, int32 SourceLen)
    {
        TCountingOutputIterator<WIDECHAR> Dest;
        int32 Result = ConvertFromUTF8<WIDECHAR>(Dest, MAX_int32, Source, SourceLen);
        return Result;
    }
    UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const WIDECHAR* Src)
    {
        int32 Written = ConvertToUTF8(Dest, DestLen, Src, FNullTerminal{});
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
    UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const WIDECHAR* Src, int32 SrcLen)
    {
        int32 Written = ConvertToUTF8(Dest, DestLen, Src, SrcLen);
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
    UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const UTF32CHAR* Src)
    {
        int32 Written = ConvertToUTF8(Dest, DestLen, Src, FNullTerminal{});
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
    UTF8CHAR* Convert(UTF8CHAR* Dest, int32 DestLen, const UTF32CHAR* Src, int32 SrcLen)
    {
        int32 Written = ConvertToUTF8(Dest, DestLen, Src, SrcLen);
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
    ANSICHAR* Convert(ANSICHAR* Dest, int32 DestLen, const UTF8CHAR* Src)
    {
        int32 Written = ConvertFromUTF8<ANSICHAR>(Dest, DestLen, Src, FNullTerminal{});
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
    ANSICHAR* Convert(ANSICHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen)
    {
        int32 Written = ConvertFromUTF8<ANSICHAR>(Dest, DestLen, Src, SrcLen);
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
    WIDECHAR* Convert(WIDECHAR* Dest, int32 DestLen, const UTF8CHAR* Src)
    {
        int32 Written = ConvertFromUTF8<WIDECHAR>(Dest, DestLen, Src, FNullTerminal{});
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
    WIDECHAR* Convert(WIDECHAR* Dest, int32 DestLen, const UTF8CHAR* Src, int32 SrcLen)
    {
        int32 Written = ConvertFromUTF8<WIDECHAR>(Dest, DestLen, Src, SrcLen);
        if (Written == -1)
        {
            return nullptr;
        }
        return Dest + Written;
    }
}
