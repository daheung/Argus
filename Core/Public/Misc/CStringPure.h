#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"
#include <stdarg.h>

CORE_C_API bool8 IsPureAscii(const char* Str);

CORE_C_API bool8 IsPureAsciiWithLen(const char* Str, const size_t StrLen);

CORE_C_API bool8 IsNumeric(const char* Str);

CORE_C_API bool8 IsEqual(const char* String1, const char* String2);

/**
 * @brief Empties the privided string by setting the first character to 0.
 *
 * @param String The string to be emptied.
 * @return A pointer to String.
 */
CORE_C_API char* StrEmpty(char* String);

CORE_C_API char* Strcpy(char* Dest, size_t DestCount, const char* Src);

CORE_C_API char* Strncpy(char* Dest, const char* Src, size_t DestCount);

CORE_C_API char* Strcat(char* Dest, size_t DestCount, const char* Src);

CORE_C_API char* Strncat(char* Dest, const char* Src, int32 MaxLen);

CORE_C_API char* Strupr(char* Dest, size_t DestCount);

CORE_C_API int32 Strcmp(const char* String1, const char* String2);

CORE_C_API int32 Strncmp(const char* String1, const char* String2, size_t Count);

CORE_C_API int32 Stricmp(const char* String1, const char* String2);

CORE_C_API int32 Strnicmp(const char* String1, const char* String2, size_t Count);

/** NOTE: Use FMemory::Free */
//CORE_C_API char* Strdup(const char* String);
//
//CORE_C_API char* Strndup(const char* String, size_t Count);

/**
 * @brief Creates a new string containing the leftmost characters of the input string.
 *
 * The returned string is newly allocated and must be released by the caller.
 *
 * @param String Null-terminated source string.
 * @param Count  Maximum number of characters to copy from the beginning.
 * @return Newly allocated null-terminated string, or NULL on failure.
 */
CORE_C_API void StrLeft(char* RESTRICT Dest, const char* RESTRICT String, size_t Count);

CORE_C_API void StrnLeft(char* RESTRICT Dest, size_t DestSize, const char* RESTRICT String, size_t Count);

/**
 * @brief Copies a substring into the destination buffer.
 *
 * Copies from Src starting at Start.
 * If Count is negative, copies to the end of Src.
 *
 * @param Dest  Destination buffer.
 * @param Src   Null-terminated source string.
 * @param Start Starting index in Src.
 * @param Count Number of characters to copy, or negative to copy to the end.
 */
CORE_C_API void StrMid(char* RESTRICT Dest, const char* RESTRICT Src, size_t Start, int32 Count);

/**
 * @brief Copies a substring into a size-limited destination buffer.
 *
 * Copies from Src starting at Start without exceeding DestSize.
 * If Count is negative, copies to the end of Src.
 *
 * @param Dest     Destination buffer.
 * @param DestSize Size of the destination buffer.
 * @param Src      Null-terminated source string.
 * @param Start    Starting index in Src.
 * @param Count    Number of characters to copy, or negative to copy to the end.
 */
CORE_C_API void StrnMid(char* RESTRICT Dest, size_t DestSize, const char* RESTRICT Src, size_t Start, int32 Count);

/**
 * @brief Creates a new string containing the rightmost characters of the input string.
 *
 * The returned string is newly allocated and must be released by the caller.
 *
 * @param String Null-terminated source string.
 * @param Count  Maximum number of characters to copy from the end.
 * @return Newly allocated null-terminated string, or NULL on failure.
 */
CORE_C_API void StrRight(char* RESTRICT Dest, const char* RESTRICT String, size_t Count);

CORE_C_API void StrnRight(char* RESTRICT Dest, size_t DestSize, const char* RESTRICT String, size_t Count);

CORE_C_API const char* Strnistr(const char* Str, int32 InStrLen, const char* Find, int32 FindLen);

CORE_C_API const char* Strnstr(const char* Str, int32 InStrLen, const char* Find, int32 FindLen);

/**
 * @brief Finds the first occurrence of a character within a string.
 *
 * Performs a case-sensitive search and returns the zero-based index
 * of the first matching character.
 *
 * @param String The null-terminated string to search.
 * @param Find The character to search for.
 * @return The zero-based index of the first occurrence of Find,
 *         or -1 if no match is found.
 */
CORE_C_API NODISCARD int32 StrFind(const char* String, char Find);

/**
 * @brief Finds the first occurrence of a substring within a string.
 *
 * Performs a case-sensitive search and returns the zero-based index
 * of the first matching substring.
 *
 * @param String The null-terminated string to search.
 * @param Find The null-terminated substring to search for.
 * @return The zero-based index of the first occurrence of Find,
 *         or -1 if no match is found.
 */
CORE_C_API NODISCARD int32 StrFindStr(const char* String, const char* Find);

/**
 * @brief Finds the last occurrence of a character within a string.
 *
 * Performs a case-sensitive search and returns the zero-based index
 * of the last matching character.
 *
 * @param String The null-terminated string to search.
 * @param Find The character to search for.
 * @return The zero-based index of the last occurrence of Find,
 *         or -1 if no match is found.
 */
CORE_C_API NODISCARD int32 StrFindLast(const char* String, char Find);

/**
 * @brief Finds the last occurrence of a substring within a string.
 *
 * Performs a case-sensitive search and returns the zero-based index
 * of the last matching substring.
 *
 * @param String The null-terminated string to search.
 * @param Find The null-terminated substring to search for.
 * @return The zero-based index of the last occurrence of Find,
 *         or -1 if no match is found.
 */
CORE_C_API NODISCARD int32 StrFindLastStr(const char* String, const char* Find);

CORE_C_API size_t Strlen(const char* String);

CORE_C_API size_t Strnlen(const char* String, size_t StringSize);

CORE_C_API const char* Strstr(const char* String, const char* Find);

CORE_C_API char* Strchr(const char* String, char c);

CORE_C_API char* Strrchr(char* String, char c);

CORE_C_API char* Strrstr(char* String, const char* Find);

/**
 * @brief Replaces all occurrences of a character in a string.
 *
 * @param String The null-terminated string to modify.
 * @param From The character to replace.
 * @param To The replacement character.
 * @return The input string, or NULL if String is NULL.
 */
CORE_C_API char* StrReplace(char* String, char From, char To);

CORE_C_API int32 Strspn(const char* String, const char* Mask);

CORE_C_API int32 Strcspn(const char* String, const char* Mask);

/**
 * @brief Removes leading and trailing whitespace characters from a string.
 *
 * Trims the string in place. Leading characters are shifted toward the
 * beginning of the buffer, preserving the original buffer address.
 *
 * @param String Null-terminated writable string to trim.
 * @return The original String pointer, or NULL if String is NULL.
 */
CORE_C_API NODISCARD char* StrTrim(char* String);

/**
 * @brief Removes leading whitespace characters from a string.
 *
 * Modifies the input string in place and returns the same buffer pointer.
 *
 * @param String Null-terminated string to trim.
 * @return Pointer to the modified string buffer.
 */
CORE_C_API NODISCARD char* StrTrimStart(char* String);

/**
 * @brief Removes trailing whitespace characters from a string.
 *
 * Modifies the input string in place and returns the same buffer pointer.
 *
 * @param String Null-terminated string to trim.
 * @return Pointer to the modified string buffer.
 */
CORE_C_API NODISCARD char* StrTrimEnd(char* String);

/**
 * @brief Converts a null-terminated string to a boolean value.
 *
 * Parses the input string as a boolean representation and writes the
 * converted value to the output parameter.
 *
 * @param String The null-terminated string to convert.
 * @param OutValue Pointer to the variable that receives the converted boolean value.
 * @return true if the conversion succeeds; otherwise false.
 */
CORE_C_API NODISCARD bool8 Atobool(const char* String, bool8* OutValue);

CORE_C_API NODISCARD int32 Atoi(const char* String);

CORE_C_API NODISCARD int64 Atoi64(const char* String);

CORE_C_API NODISCARD float32 Atof(const char* String);

CORE_C_API NODISCARD float64 Atod(const char* String);

CORE_C_API NODISCARD int32 Strtoi(const char* Start, char** End, int32 Base);

CORE_C_API NODISCARD int64 Strtoi64(const char* Start, char** End, int32 Base);

CORE_C_API NODISCARD uint64 Strtoui64(const char* Start, char** End, int32 Base);

CORE_C_API NODISCARD char* Strtok(char* TokenString, const char* Delim, char** Context);

CORE_C_API NODISCARD int32 GetVarArgs(char* Dest, size_t DestSize, const char* Fmt, va_list ArgPtr);

/**
 * @brief Calculates the length of a formatted string.
 *
 * @param Format The null-terminated format string.
 * @param Args The variable argument list referenced by the format string.
 * @returns The number of characters required to store the formatted string,
 *          excluding the null terminator, or a negative value on failure.
 */
CORE_C_API NODISCARD int32 GetVarArgsLength(const char* Format, va_list Args);

/** Performs string formatting to dest given format string and parameters. */
CORE_C_API int32 FormatString(char* Dest, size_t DestSize, const char* Fmt, ...);

/** Performs variadic formatting to dest given format string and va_list.
 * @param Dest The destination for the formatted string.
 * @param Format The string to be formatted.
 * @param ArgPtr The variadic argument list.
 * @returns The size of the data written.
 */
CORE_C_API int32 FormatStringV(char* Dest, size_t DestSize, const char* Format, va_list ArgPtr);
