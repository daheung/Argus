#pragma once

#if defined(__clang__) || defined(__gcc__)
    #define STATIC_ASSERT _Static_assert
#else 
    #define STATIC_ASSERT static_assert
#endif

#ifndef TRUE
    #define TRUE 1
#endif
#ifndef FALSE
    #define FALSE 0
#endif
#ifndef NULL
    #define NULL 0
#endif
#ifndef RESTRICT
    #define RESTRICT
#endif

/** Platform-specific definitions */
#if defined(_KERNEL_MODE) || defined(_KERNEL_MODE)
    // Specific kernel driver definitions
    #define PLATFORM_WDM  1
    #define PLATFORM_MKDF 1
#endif

#if defined(_WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define PLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit Windows is not supported"
    #endif
#endif

#if defined(__linux__) || defined(__gnu_linux__)
    #define PLATFORM_LINUX 1
#endif

#if defined(__ANDROID__)
    #define PLATFORM_ANDROID 1  
#endif

#if defined(__unix__)
    #define PLATFORM_UNIX 1
#endif

#if defined(__POSIX_VERSION)
    #define PLATFORM_POSIX 1
#endif

#if __APPLE__
    #define PLATFORM_APPLE 1
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
        #define PLATFORM_IOS 1
        #define PLATFORM_IOS_SIMULATOR 1
    #elif TARGET_OS_IPHONE
        #define PLATFORM_IOS 1
    #elif TARGET_OS_MAC
    #else
        #error "Unknown Apple platform"
    #endif
#endif 

/* C/C++ linkage */
#ifdef __cplusplus
    #define CORE_EXTERN_C extern "C"
    #define CORE_EXTERN_C_BEGIN extern "C" {
    #define CORE_EXTERN_C_END }
#else
    #define CORE_EXTERN_C
    #define CORE_EXTERN_C_BEGIN
    #define CORE_EXTERN_C_END
#endif

#ifdef CORE_EXPORT
#ifdef _MSC_VER
    #define CORE_API __declspec(dllexport)
#else
    #define CORE_API __attribute__((visibility("default")))
#endif
#elif CORE_IMPORT
#ifdef _MSC_VER
    #define CORE_API __declspec(dllimport)
#else
    #define CORE_API
#endif
#else
    // Library project
    #define CORE_API
#endif

#define CORE_C_API CORE_EXTERN_C CORE_API
#define CORE_CPP_API CORE_API

/** Function type macros. */
#ifndef VARARGS
    #define VARARGS     __cdecl
#endif
#ifndef CDECL
    #define CDECL       __cdecl
#endif
#ifndef STDCALL
    #define STDCALL     __stdcall
#endif
#ifndef FASTCALL
    #define FASTCALL    __fastcall
#endif
#ifndef INLINE
    #define INLINE        inline
#endif
#ifndef FORCEINLINE
    #define FORCEINLINE __forceinline
#endif
#ifndef NOINLINE
    #define NOINLINE    __declspec(noinline)
#endif

#ifndef NORETURN
    #define NORETURN    [[noreturn]]
#endif
#ifndef NODISCARD
    #define NODISCARD   [[nodiscard]]
#endif

#ifndef DEPRECATED
    #define DEPRECATED(Text)
#endif
#ifndef UNUSED_PARAMETER
    #define UNUSED_PARAMETER(Param) ((void)(Param))
#endif
#ifndef UNREFERENCED_PARAMETER
    #define UNREFERENCED_PARAMETER(Param) ((void)(Param))
#endif

/** 
 * @brief Any id set to this should be considered invalid,
 * and not actually pointing to a real object.
 */
#define INVALID_ID ((uint32)0xFFFFFFFF)

#define _CRT_SECURE_NO_WARNINGS

enum { INDEX_NONE = -1 };

#define ReturnIfFalse(expr)				\
    do									\
    {									\
        const bool bSuccess = (expr);	\
        if (!bSuccess)                  \
        {								\
            return false;				\
        }								\
    } while (false)