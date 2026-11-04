#include "Core/Public/Misc/GenericStricmp.h"
#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Char.h"

static const uint8 LowerAscii[128] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
};

static FORCEINLINE bool8 BothAscii(uint32 C1, uint32 C2)
{
    return ((C1 | C2) & 0xFFFFFF80) == 0;
}

namespace Argus::String
{
    template <typename CharTypeA, typename CharTypeB>
    int32 StricmpImpl(const CharTypeA* String1, const CharTypeB* String2)
    {
        while (TRUE)
        {
            CharTypeA C1 = *String1++;
            CharTypeB C2 = *String2++;

            uint32 U1 = TChar<CharTypeA>::ToUnsigned(C1);
            uint32 U2 = TChar<CharTypeB>::ToUnsigned(C2);

            if (U1 == U2)
            {
                if (U1)
                {
                    continue;
                }

                return 0;
            }
            else if (BothAscii(U1, U2))
            {
                uint32 Diff = LowerAscii[U1] - LowerAscii[U2];
                if (Diff)
                {
                    return Diff;
                }
            }
            else
            {
                return U1 - U2;
            }
        }
    }

    template <typename CharTypeA, typename CharTypeB>
    int32 StrnicmpImpl(const CharTypeA* String1, const CharTypeB* String2, size_t Count)
    {
        for (; Count > 0; --Count)
        {
            CharTypeA C1 = *String1++;
            CharTypeB C2 = *String2++;

            uint32 U1 = TChar<CharTypeA>::ToUnsigned(C1);
            uint32 U2 = TChar<CharTypeB>::ToUnsigned(C2);

            if (U1 == U2)
            {
                if (U1)
                {
                    continue;
                }

                return 0;
            }
            else if (BothAscii(U1, U2))
            {
                int32 Diff = LowerAscii[U1] - LowerAscii[U2];

                if (Diff)
                {
                    return Diff;
                }
            }
            else
            {
                return U1 - U2;
            }
        }

        return 0;
    }
}

int32 FGenericStricmp::Stricmp(const ANSICHAR*  String1, const ANSICHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const WIDECHAR*  String1, const WIDECHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const UTF8CHAR*  String1, const UTF8CHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const UTF32CHAR* String1, const UTF32CHAR* String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const ANSICHAR*  String1, const WIDECHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const ANSICHAR*  String1, const UTF8CHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const ANSICHAR*  String1, const UTF32CHAR* String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const WIDECHAR*  String1, const ANSICHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const UTF8CHAR*  String1, const ANSICHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }
int32 FGenericStricmp::Stricmp(const UTF32CHAR* String1, const ANSICHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }

int32 FGenericStricmp::Strnicmp(const ANSICHAR*  String1, const ANSICHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const WIDECHAR*  String1, const WIDECHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const UTF8CHAR*  String1, const UTF8CHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const UTF32CHAR* String1, const UTF32CHAR* String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const ANSICHAR*  String1, const WIDECHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const ANSICHAR*  String1, const UTF8CHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const ANSICHAR*  String1, const UTF32CHAR* String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const WIDECHAR*  String1, const ANSICHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const UTF8CHAR*  String1, const ANSICHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
int32 FGenericStricmp::Strnicmp(const UTF32CHAR* String1, const ANSICHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }

//int32 FGenericStricmp::Stricmp(const UTF16CHAR* String1, const UTF16CHAR* String2) { return Argus::String::StricmpImpl(String1, String2); }
//int32 FGenericStricmp::Stricmp(const ANSICHAR*  String1, const UTF16CHAR* String2) { return Argus::String::StricmpImpl(String1, String2); }
//int32 FGenericStricmp::Stricmp(const UTF16CHAR* String1, const ANSICHAR*  String2) { return Argus::String::StricmpImpl(String1, String2); }

//int32 FGenericStricmp::Strnicmp(const UTF16CHAR* String1, const UTF16CHAR* String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
//int32 FGenericStricmp::Strnicmp(const ANSICHAR*  String1, const UTF16CHAR* String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
//int32 FGenericStricmp::Strnicmp(const UTF16CHAR* String1, const ANSICHAR*  String2, size_t Count) { return Argus::String::StrnicmpImpl(String1, String2, Count); }
