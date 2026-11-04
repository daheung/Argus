#pragma once

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"

/** Disable assertions by commenting out the below line. */
#ifndef CORE_ASSERTIONS_ENABLED
    #define CORE_ASSERTIONS_ENABLED 1 
#endif

CORE_C_API void ReportAssertionFailure(const char* Expression, const char* Message, const char* File, int32 Line);
CORE_C_API void ReportAssertionFailureFmt(const char* Expression, const char* Message, const char* File, int32 Line, ...);

#if defined(_MSC_VER)
    #include <intrin.h>

    // Q: Why is there a __nop() before __debugbreak()?
    // A: VS' debug engine has a bug where it will silently swallow explicit
    // breakpoint interrupts when single-step debugging either line-by-line or
    // over call instructions. This can hide legitimate reasons to trap. Asserts
    // for example, which can appear as if the did not fire, leaving a programmer
    // unknowingly debugging an undefined process.
    // Referenced to: Unreal Engine's PLATFORM_BREAK macro.
    #define DEBUG_BREAK() (__nop(), __debugbreak())
#elif defined(__clang__) && __has_builtin(__builtin_debugtrap)
    #define DEBUG_BREAK() __builtin_debugtrap()
#elif defined(__GNUC__) || defined(__clang__)
    #include <csignal>
    #define DEBUG_BREAK() raise(SIGTRAP)
#endif

#define CORE_CHECK(Expression)                                                                     \
    do                                                                                             \
    {                                                                                              \
        if (!(Expression))                                                                         \
        {                                                                                          \
            ReportAssertionFailureFmt(#Expression, "", __FILE__, __LINE__);                        \
            DEBUG_BREAK();                                                                         \
        }                                                                                          \
    } while (0)

#define CORE_CHECKF(Expression, Message, ...)                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(Expression))                                                                         \
        {                                                                                          \
            ReportAssertionFailureFmt(#Expression, Message, __FILE__, __LINE__, ##__VA_ARGS__);    \
            DEBUG_BREAK();                                                                         \
        }                                                                                          \
    } while (0)

#if CORE_ASSERTIONS_ENABLED
#define CORE_ASSERT(Expression)                                                 \
    do {                                                                        \
        if (!(Expression)) {                                                    \
            ReportAssertionFailure(#Expression, "", __FILE__, __LINE__);        \
            DEBUG_BREAK();                                                      \
        }                                                                       \
    } while (0)

#define CORE_ASSERT_MSG(Expression, Message)                                    \
    do {                                                                        \
        if (!(Expression)) {                                                    \
            ReportAssertionFailure(#Expression, Message, __FILE__, __LINE__);   \
            DEBUG_BREAK();                                                      \
        }                                                                       \
    } while (0)

#if defined(_DEBUG) || defined(D_DEBUG)
#define CORE_ASSERT_DEBUG(Expression)                                           \
    do {                                                                        \
        if (!(Expression)) {                                                    \
            ReportAssertionFailure(#Expression, "", __FILE__, __LINE__);        \
            DEBUG_BREAK();                                                      \
        }                                                                       \
    } while (0)
#else
#define CORE_ASSERT_DEBUG(Expression)
#endif

#else
#define CORE_ASSERT(Expression)
#define CORE_ASSERT_MSG(Expression, Message)
#define CORE_ASSERT_DEBUG(Expression)
#endif

#define check (expr) CORE_CHECK (expr)
#define checkf(expr, msg, ...) CORE_CHECKF(expr, msg, ...)