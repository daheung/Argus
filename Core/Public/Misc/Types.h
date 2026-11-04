#include "Core/Public/Misc/Defines.h"

#include <stddef.h>

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

typedef signed char         int8;
typedef signed short        int16;
typedef signed int          int32;
typedef signed long long    int64;

typedef float               float32;
typedef double              float64;

typedef int                 bool32;
typedef char                bool8;

typedef char                ANSICHAR;
typedef wchar_t             WIDECHAR;
typedef uint8               UTF8CHAR;
typedef uint16              UTF16CHAR;
typedef uint32              UTF32CHAR;

typedef decltype(nullptr)   TYPE_OF_NULLPTR;


STATIC_ASSERT(sizeof(uint8 ) == 1, "Expected uint8 to be 1 byte");
STATIC_ASSERT(sizeof(uint16) == 2, "Expected uint16 to be 2 bytes");
STATIC_ASSERT(sizeof(uint32) == 4, "Expected uint32 to be 4 bytes");
STATIC_ASSERT(sizeof(uint64) == 8, "Expected uint64 to be 8 bytes");

STATIC_ASSERT(sizeof(int8 ) == 1, "Expected int8 to be 1 byte");
STATIC_ASSERT(sizeof(int16) == 2, "Expected int16 to be 2 bytes");
STATIC_ASSERT(sizeof(int32) == 4, "Expected int32 to be 4 bytes");
STATIC_ASSERT(sizeof(int64) == 8, "Expected int64 to be 8 bytes");

STATIC_ASSERT(sizeof(float32) == 4, "Expected float32 to be 4 bytes");
STATIC_ASSERT(sizeof(float64) == 8, "Expected float64 to be 8 bytes");