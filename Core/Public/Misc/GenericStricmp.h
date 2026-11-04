#pragma once

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"

class FGenericStricmp
{
public:
	static int32 Stricmp(const ANSICHAR* String1, const ANSICHAR* String2);
	static int32 Stricmp(const WIDECHAR* String1, const WIDECHAR* String2);
	static int32 Stricmp(const UTF8CHAR* String1, const UTF8CHAR* String2);
	static int32 Stricmp(const UTF32CHAR* String1, const UTF32CHAR* String2);

	static int32 Stricmp(const ANSICHAR* String1, const WIDECHAR* String2);
	static int32 Stricmp(const ANSICHAR* String1, const UTF8CHAR* String2);
	static int32 Stricmp(const ANSICHAR* String1, const UTF32CHAR* String2);
	static int32 Stricmp(const WIDECHAR* String1, const ANSICHAR* String2);
	static int32 Stricmp(const UTF8CHAR* String1, const ANSICHAR* String2);
	static int32 Stricmp(const UTF32CHAR* String1, const ANSICHAR* String2);

	static int32 Strnicmp(const ANSICHAR* String1, const ANSICHAR* String2, size_t Count);
	static int32 Strnicmp(const WIDECHAR* String1, const WIDECHAR* String2, size_t Count);
	static int32 Strnicmp(const UTF8CHAR* String1, const UTF8CHAR* String2, size_t Count);
	static int32 Strnicmp(const UTF32CHAR* String1, const UTF32CHAR* String2, size_t Count);

	static int32 Strnicmp(const ANSICHAR* String1, const WIDECHAR* String2, size_t Count);
	static int32 Strnicmp(const ANSICHAR* String1, const UTF8CHAR* String2, size_t Count);
	static int32 Strnicmp(const ANSICHAR* String1, const UTF32CHAR* String2, size_t Count);
	static int32 Strnicmp(const WIDECHAR* String1, const ANSICHAR* String2, size_t Count);
	static int32 Strnicmp(const UTF8CHAR* String1, const ANSICHAR* String2, size_t Count);
	static int32 Strnicmp(const UTF32CHAR* String1, const ANSICHAR* String2, size_t Count);

#if !defined(PLATFORM_WDM) && !defined(PLATFORM_KMDF)
	// NOTE: In the WDK build, WIDECHAR and UTF16CHAR are the same type.
	//static int32 Stricmp(const UTF16CHAR* String1, const UTF16CHAR* String2);
	//static int32 Stricmp(const ANSICHAR* String1, const UTF16CHAR* String2);
	//static int32 Stricmp(const UTF16CHAR* String1, const ANSICHAR* String2);
	//static int32 Strnicmp(const UTF16CHAR* String1, const UTF16CHAR* String2, size_t Count);
	//static int32 Strnicmp(const ANSICHAR* String1, const UTF16CHAR* String2, size_t Count);
	//static int32 Strnicmp(const UTF16CHAR* String1, const ANSICHAR* String2, size_t Count);
#endif
};
