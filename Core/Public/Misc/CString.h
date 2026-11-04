#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/GenericStricmp.h"

template <typename T>
struct TCString
{
	typedef T CharType;

	NODISCARD static FORCEINLINE int32 Strlen(const CharType* Str) noexcept
	{
        int32 Length = 0;
        while (true)
        {
            if (Str[Length] == (CharType)0)
            {
                return Length;
            }

            ++Length;
        }

        return Length;
	}

    NODISCARD static FORCEINLINE int32 Strcmp(const CharType* String1, const CharType* String2) noexcept
    {
        size_t Index = 0;
        while (true)
        {
            const CharType C1 = String1[Index];
            const CharType C2 = String2[Index];
            
            if (C1 != C2)
            {
                return static_cast<int32>(C1 - C2);
            }
            if (C1 == (CharType)0)
            {
                return 0;
            }

            ++Index;
        }

        return 0;
    }

    NODISCARD static FORCEINLINE size_t Strncmp(const CharType* String1, const CharType* String2, size_t Count) noexcept
    {
        size_t Index = 0;
        while (Index < Count)
        {
            const CharType C1 = String1[Index];
            const CharType C2 = String2[Index];

            if (C1 != C2)
            {
                return static_cast<int32>(C1 - C2);
            }
            if (C1 == (CharType)0)
            {
                return 0;
            }

            ++Index;
        }

        return 0;
    }

    NODISCARD static FORCEINLINE int32 Stricmp(const CharType* String1, const CharType* String2) noexcept
    {
        return FGenericStricmp::Stricmp(String1, String2);
    }

    NODISCARD static FORCEINLINE int32 Strnicmp(const CharType* String1, const CharType* String2, size_t Count) noexcept
    {
        return FGenericStricmp::Strnicmp(String1, String2, Count);
    }
};
