#include "Core/Public/Misc/CStringPure.h"
#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Assertion.h"
#include "Core/Public/Misc/Char.h"
#include "Core/Public/Math/MathFwd.h"
#include "Core/Public/Memory/Memory.h"
#include "Core/Public/Misc/GenericStricmp.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>


CORE_C_API bool8 IsPureAscii(const char* Str)
{
    for (; *Str; ++Str)
    {
        if ((uint8)*Str >= 0x80)
        {
            return FALSE;
        }
    }

    return TRUE;
}

CORE_C_API bool8 IsPureAsciiWithLen(const char* Str, const size_t StrLen)
{
    for (size_t Index = 0; Index < StrLen; ++Index, ++Str)
    {
        if ((uint8)*Str >= 0x80)
        {
            return FALSE;
        }
    }

    return TRUE;
}

CORE_C_API bool8 IsNumeric(const char* Str)
{
    if (*Str == '-' || *Str == '+')
    {
        Str++;
    }

    bool8 bHasDot = FALSE;
    while (*Str != '\0')
    {
        if (*Str == '.')
        {
            if (bHasDot)
            {
                return FALSE;
            }
            bHasDot = TRUE;
        }
        else if (!TChar<ANSICHAR>::IsDigit(*Str))
        {
            return FALSE;
        }

        ++Str;
    }

    return TRUE;
}

/** Case-sensitive string comparison. True if the same, otherwise false. */
CORE_C_API bool8 IsEqual(const char* String1, const char* String2)
{
    return Strcmp(String1, String2) == 0;
}

CORE_C_API char* StrEmpty(char* String)
{
    if (String)
    {
        String[0] = '\0';
    }

    return String;
}

CORE_C_API char* Strcpy(char* Dest, size_t DestCount, const char* Src)
{
    UNREFERENCED_PARAMETER(DestCount);
    return (char*)strcpy(Dest, Src);
}

CORE_C_API char* Strncpy(char* Dest, const char* Src, size_t DestCount)
{
    return (char*)strncpy(Dest, Src, DestCount);
}

CORE_C_API char* Strcat(char* Dest, size_t DestCount, const char* Src)
{
    UNREFERENCED_PARAMETER(DestCount);
    return (char*)strcat(Dest, Src);
}

CORE_C_API char* Strncat(char* Dest, const char* Src, int32 MaxLen)
{
    int32 Len = (int32)Strlen(Dest);
    char* NewDest = Dest + Len;

    if ((MaxLen -= Len) > 0)
    {
        Strncpy(NewDest, Src, MaxLen);
    }

    return Dest;
}

CORE_C_API char* Strupr(char* Dest, size_t DestCount)
{
    for (char* Char = Dest; *Char && DestCount > 0; ++Char, --DestCount)
    {
        *Char = TChar<ANSICHAR>::ToUpper(*Char);
    }

    return Dest;
}

CORE_C_API int32 Strcmp(const char* String1, const char* String2)
{
    return (int32)strcmp(String1, String2);
}

CORE_C_API int32 Strncmp(const char* String1, const char* String2, size_t Count)
{
    return (int32)strncmp(String1, String2, Count);
}

CORE_C_API int32 Stricmp(const char* String1, const char* String2)
{
    return FGenericStricmp::Stricmp(String1, String2);
}

CORE_C_API int32 Strnicmp(const char* String1, const char* String2, size_t Count)
{
    return FGenericStricmp::Strnicmp(String1, String2, Count);
}

/** NOTE:: Use FMemory::Free */
//CORE_C_API char* Strdup(const char* String)
//{
//    const size_t Length = Strlen(String);
//    char* OutString = static_cast<char*>(FMemory::Malloc((Length + 1) * sizeof(char)));
//    FMemory::Memcpy(OutString, String, Length + 1);
//    return OutString;
//}
//
//CORE_C_API char* Strndup(const char* String, size_t Count)
//{
//    const size_t Length = Strnlen(String, Count);
//    char* OutString = static_cast<char*>(FMemory::Malloc((Length + 1) * sizeof(char)));
//    FMemory::Memcpy(OutString, String, Length);
//    OutString[Length] = '\0';
//    return OutString;
//}

CORE_C_API void StrLeft(char* RESTRICT Dest, const char* RESTRICT String, size_t Count)
{
    FMemory::Memcpy(Dest, String, Count);
    Dest[Count] = 0;
}

CORE_C_API void StrMid(char* RESTRICT Dest, const char* RESTRICT Src, size_t Start, int32 Count)
{
    CORE_CHECKF(Dest, "Dest must not be NULL.");
    CORE_CHECKF(Src, "Src must not be NULL.");

    //const size_t SrcLength = Strlen(Src);
    //if (Start >= SrcLength)
    //{
    //    Dest[0] = 0;
    //    return;
    //}

    //size_t I = Start;
    //size_t J = 0;

    //if (Count >= 0)
    //{
    //    for (; J < (size_t)Count && Src[I]; ++I, ++J)
    //    {
    //        Dest[J] = Src[I];
    //    }
    //}
    //else
    //{
    //    /** If a negative value is passed, proceed to the end of the string. */
    //    for (; Src[I]; ++I, ++J)
    //    {
    //        Dest[J] = Src[I];
    //    }

    //}

    //Dest[J] = 0;

    const size_t SrcLength = Strlen(Src);
    if (Start >= SrcLength)
    {
        Dest[0] = 0;
        return;
    }

    const size_t Remaining = SrcLength - Start;
    const size_t Length = Count >= 0
        ? CoreMin((size_t)Count, Remaining)
        : Remaining;

    FMemory::Memcpy(Dest, Src + Start, Length);
    Dest[Length] = 0;
}

CORE_C_API void StrnMid(char* RESTRICT Dest, size_t DestSize, const char* RESTRICT Src, size_t Start, int32 Count)
{
    CORE_CHECKF(Dest, "Dest must not be NULL.");
    CORE_CHECKF(Src, "Src must not be NULL.");

    //if (DestSize == 0)
    //{
    //    return;
    //}

    //const size_t SrcLength = Strlen(Src);
    //if (Start >= SrcLength)
    //{
    //    Dest[0] = 0;
    //    return;
    //}

    //size_t I = Start;
    //size_t J = 0;

    //if (Count >= 0)
    //{
    //    for (; J < (size_t)Count && J + 1 < DestSize && Src[I]; ++I, ++J)
    //    {
    //        Dest[J] = Src[I];
    //    }
    //}
    //else
    //{
    //    /** If a negative value is passed, proceed to the end of the string. */
    //    for (; J + 1 < DestSize && Src[I]; ++I, ++J)
    //    {
    //        Dest[J] = Src[I];
    //    }

    //}

    //Dest[J] = 0;

    if (DestSize == 0)
    {
        return;
    }

    const size_t SrcLength = Strlen(Src);
    if (Start >= SrcLength)
    {
        Dest[0] = 0;
        return;
    }

    const size_t Remaining = SrcLength - Start;
    const size_t CopyLength = Count >= 0
        ? CoreMin((size_t)Count, Remaining)
        : Remaining;

    const size_t Length =
        CoreMin(CopyLength, DestSize - 1);

    FMemory::Memcpy(Dest, Src + Start, Length);
    Dest[Length] = 0;
}

CORE_C_API void StrRight(char* RESTRICT Dest, const char* RESTRICT String, size_t Count)
{
    const size_t Length = Strlen(String);
    FMemory::Memcpy(Dest, String + (Length - Count), Count);
    Dest[Count] = 0;
}

CORE_C_API void StrnRight(char* RESTRICT Dest, size_t DestSize, const char* RESTRICT String, size_t Count)
{
    UNREFERENCED_PARAMETER(Dest);
    UNREFERENCED_PARAMETER(DestSize);
    UNREFERENCED_PARAMETER(String);
    UNREFERENCED_PARAMETER(Count);
}

CORE_C_API const char* Strnistr(const char* Str, int32 InStrLen, const char* Find, int32 FindLen)
{
    if (FindLen <= 0)
    {
        //CORE_LOG(FindLen >= 0, LOG_LEVEL_ERROR, "Invalid FindLen : %d", FindLen);
        return Str;
    }
    if (InStrLen < FindLen)
    {
        //CORE_LOG(InStrLen >= 0, LOG_LEVEL_ERROR, "Invalid InStrLen : %d", InStrLen);
        return NULL;
    }

    char FindInitial = TChar<ANSICHAR>::ToUpper(*Find);
    int32 FindSuffixLength = FindLen - 1;
    const char* FindSuffix = Find + 1;

    const char* StrLastChance = Str + InStrLen - FindLen;
    while (Str <= StrLastChance)
    {
        char StrChar = *Str++;
        StrChar = TChar<ANSICHAR>::ToUpper(StrChar);
        if (StrChar == FindInitial && !Strnicmp(Str, FindSuffix, FindSuffixLength))
        {
            return Str - 1;
        }
    }

    return NULL;
}

CORE_C_API const char* Strnstr(const char* Str, int32 InStrLen, const char* Find, int32 FindLen)
{
    if (FindLen <= 0)
    {
        //CORE_LOG(FindLen >= 0, LOG_LEVEL_ERROR, "Invalid FindLen : %d", FindLen);
        return Str;
    }
    if (InStrLen < FindLen)
    {
        //CORE_LOG(InStrLen >= 0, LOG_LEVEL_ERROR, "Invalid InStrLen : %d", InStrLen);
        return NULL;
    }

    char FindInital = *Find;
    int32 FindSuffixLength = FindLen - 1;
    const char* FindSuffix = Find + 1;
    const char* StrLastChance = Str + InStrLen - FindLen;

    while (Str <= StrLastChance)
    {
        char StrChar = *Str++;
        if (StrChar == FindInital && !Strncmp(Str, FindSuffix, FindSuffixLength))
        {
            return Str - 1;
        }
    }

    return NULL;
}

CORE_C_API int32 StrFind(const char* String, char Find)
{
    if (!String)
    {
        return -1;
    }
    if (Find == '\0')
    {
        return (int32)Strlen(String);
    }

    for (int32 Index = 0; String[Index]; ++Index)
    {
        if (String[Index] == Find)
        {
            return Index;
        }
    }

    return -1;
}

CORE_C_API int32 StrFindStr(const char* String, const char* Find)
{
    if (!String || !Find)
    {
        return -1;
    }

    const size_t Length = Strlen(String);
    const size_t FindLength = Strlen(Find);

    if (FindLength == 0)
    {
        return (int32)Length;
    }
    if (Length < FindLength)
    {
        return -1;
    }
    if (Length == FindLength)
    {
        return !Strncmp(String, Find, FindLength) ? 0 : -1;
    }

    for (int32 Index = 0; String[Index] && Index <= (int32)(Length - FindLength); ++Index)
    {
        if (!Strncmp(&String[Index], Find, FindLength))
        {
            return Index;
        }
    }

    return -1;
}

CORE_C_API int32 StrFindLast(const char* String, char Find)
{
    if (!String)
    {
        return -1;
    }

    const size_t Length = Strlen(String);
    if (Find == '\0')
    {
        return (int32)Length;
    }

    for (int32 Index = (int32)Length - 1; Index >= 0; --Index)
    {
        if (String[Index] == Find)
        {
            return Index;
        }
    }

    return -1;
}

CORE_C_API int32 StrFindLastStr(const char* String, const char* Find)
{
    if (!String || !Find)
    {
        return -1;
    }

    const size_t Length = Strlen(String);
    const size_t FindLength = Strlen(Find);

    if (FindLength == 0)
    {
        return (int32)Length;
    }
    if (Length < FindLength)
    {
        return -1;
    }
    if (Length == FindLength)
    {
        return !Strncmp(String, Find, FindLength) ? 0 : -1;
    }

    for (int32 Index = (int32)(Length - FindLength); Index >= 0; --Index)
    {
        if (!Strncmp(&String[Index], Find, FindLength))
        {
            return Index;
        }
    }

    return -1;
}

CORE_C_API size_t Strlen(const char* String)
{
    return (size_t)strlen(String);
}

CORE_C_API size_t Strnlen(const char* String, size_t StringSize)
{
    size_t Length = 0;

    while (Length < StringSize && String[Length] != '\0')
    {
        ++Length;
    }

    return Length;
}

CORE_C_API const char* Strstr(const char* String, const char* Find)
{
    return (const char*)strstr(String, Find);
}

CORE_C_API char* Strchr(const char* String, char c)
{
    return (char*)strchr(String, c);
}

CORE_C_API char* Strrchr(char* String, char c)
{
    return (char*)strrchr(String, c);
}

CORE_C_API char* Strrstr(char* String, const char* Find)
{
    if (*Find == (char)0)
    {
        return String + Strlen(String);
    }

    char* Result = NULL;
    for (;;)
    {
        char* Found = (char*)Strstr(String, Find);
        if (!Found)
        {
            return Result;
        }

        Result = Found;
        String = Found + 1;
    }
}

CORE_C_API char* StrReplace(char* String, char From, char To)
{
    if (!String)
    {
        return NULL;
    }

    for (char* CodePoint = String; *CodePoint; ++CodePoint)
    {
        if (*CodePoint == From)
        {
            *CodePoint = To;
        }
    }

    return String;
}

CORE_C_API int32 Strspn(const char* String, const char* Mask)
{
    const char* StringIt = String;
    while (*StringIt)
    {
        for (const char* MaskIt = Mask; *MaskIt; ++MaskIt)
        {
            if (*StringIt == *MaskIt)
            {
                goto NextChar;
            }
        }

        return (int32)(StringIt - String);

    NextChar:
        ++StringIt;
    }

    return (int32)(StringIt - String);
}

CORE_C_API int32 Strcspn(const char* String, const char* Mask)
{
    const char* StringIt = String;
    while (*StringIt)
    {
        for (const char* MaskIt = Mask; *MaskIt; ++MaskIt)
        {
            if (*StringIt == *MaskIt)
            {
                return (int32)(StringIt - String);
            }
        }

        ++StringIt;
    }

    return (int32)(StringIt - String);
}

CORE_C_API char* StrTrim(char* String)
{
    if (!String)
    {
        return NULL;
    }
    if (*String == '\0')
    {
        return String;
    }

    const size_t Length = Strlen(String);
    char* Start = String;
    char* End = &String[Length - 1];

    while (TChar<ANSICHAR>::IsWhitespace(*Start))
    {
        Start++;
    }

    while (End >= Start && TChar<ANSICHAR>::IsWhitespace(*End))
    {
        End--;
    }

    if (End < Start)
    {
        String[0] = '\0';
        return String;
    }

    const size_t NewLength = (size_t)(End - Start) + 1;
    FMemory::Memmove(String, Start, NewLength);
    String[NewLength] = '\0';

    return String;
}

CORE_C_API char* StrTrimStart(char* String)
{
    if (!String)
    {
        return NULL;
    }
    if (*String == '\0')
    {
        return String;
    }

    const size_t Length = Strlen(String);
    char* Start = String;

    while (TChar<ANSICHAR>::IsWhitespace(*Start))
    {
        Start++;
    }

    const size_t NewLength = Length - (size_t)(Start - String);
    FMemory::Memmove(String, Start, NewLength);
    String[NewLength] = '\0';

    return String;
}

CORE_C_API char* StrTrimEnd(char* String)
{
    if (!String)
    {
        return NULL;
    }
    if (*String == '\0')
    {
        return String;
    }

    const size_t Length = Strlen(String);
    char* End = &String[Length - 1];

    while (End >= String && TChar<ANSICHAR>::IsWhitespace(*End))
    {
        End--;
    }

    *End = 0;

    return String;
}

CORE_C_API bool8 Atobool(const char* String, bool8* OutValue)
{
    if (!String)
    {
        return FALSE;
    }

    if (Stricmp(String, "1") == 0 || Stricmp(String, "TRUE") == 0)
    {
        *OutValue = TRUE;
        return TRUE;
    }

    if (Stricmp(String, "0") == 0 || Stricmp(String, "FALSE") == 0)
    {
        *OutValue = FALSE;
        return TRUE;
    }

    return FALSE;
}

CORE_C_API int32 Atoi(const char* String)
{
    return atoi(String);
}

CORE_C_API int64 Atoi64(const char* String)
{
    return (int64)strtoll(String, NULL, 10);
}

CORE_C_API float32 Atof(const char* String)
{
    return strtof(String, NULL);
}

CORE_C_API float64 Atod(const char* String)
{
    return strtod(String, NULL);
}

CORE_C_API int32 Strtoi(const char* Start, char** End, int32 Base)
{
    return strtol(Start, End, Base);
}

CORE_C_API int64 Strtoi64(const char* Start, char** End, int32 Base)
{
    return (int64)strtoll(Start, End, Base);
}

CORE_C_API uint64 Strtoui64(const char* Start, char** End, int32 Base)
{
    return (uint64)strtoull(Start, End, Base);
}

CORE_C_API char* Strtok(char* TokenString, const char* Delim, char** Context)
{
    char* Current = TokenString ? TokenString : *Context;

    if (!Current)
    {
        return NULL;
    }

    Current += Strspn(Current, Delim);

    if (*Current == '\0')
    {
        *Context = NULL;
        return NULL;
    }

    char* Token = Current;

    Current += Strcspn(Current, Delim);

    if (*Current != '\0')
    {
        *Current = '\0';
        *Context = Current + 1;
    }
    else
    {
        *Context = NULL;
    }

    return Token;
}

CORE_C_API int32 GetVarArgs(char* Dest, size_t DestSize, const char* Fmt, va_list ArgPtr)
{
    int32 Result = vsnprintf(Dest, DestSize, Fmt, ArgPtr);
    return Result;
}

CORE_C_API int32 GetVarArgsLength(const char* Format, va_list Args)
{
    va_list ArgsCopy;
    va_copy(ArgsCopy, Args);
    int32 Length = vsnprintf(NULL, 0, Format, ArgsCopy);
    va_end(ArgsCopy);

    return Length;
}

CORE_C_API int32 FormatString(char* Dest, size_t DestSize, const char* Fmt, ...)
{
    if (!Dest)
    {
        return -1;
    }

    va_list ArgPtr;
    va_start(ArgPtr, Fmt);
    const int32 Result = FormatStringV(Dest, DestSize, Fmt, ArgPtr);
    va_end(ArgPtr);

    return Result;
}

CORE_C_API int32 FormatStringV(char* Dest, size_t DestSize, const char* Format, va_list ArgPtr)
{
    return GetVarArgs(Dest, DestSize, Format, ArgPtr);
}
