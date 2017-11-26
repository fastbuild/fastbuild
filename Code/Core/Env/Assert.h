// Assert.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Defines
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #define BREAK_IN_DEBUGGER __debugbreak();
#elif defined( __APPLE__ )
    #define BREAK_IN_DEBUGGER __builtin_trap();
#elif defined( __LINUX__ )
    #define BREAK_IN_DEBUGGER __asm__ __volatile__("int $3")
#else
    #error Unknown platform
#endif

// Global functions
//------------------------------------------------------------------------------
bool IsDebuggerAttached();

// DEBUG
//------------------------------------------------------------------------------
#ifdef DEBUG
    #define ASSERTS_ENABLED

    // standard assertion macro
    #define ASSERT( expression )                                                \
        do {                                                                    \
        PRAGMA_DISABLE_PUSH_MSVC(4127)                                          \
            if ( !( expression ) )                                              \
            {                                                                   \
                if ( AssertHandler::Failure( #expression, __FILE__, __LINE__ ) )\
                {                                                               \
                    BREAK_IN_DEBUGGER;                                              \
                }                                                               \
            }                                                                   \
        } while ( false )                                                       \
        PRAGMA_DISABLE_POP_MSVC

    // standard assertion macro with message
    #define ASSERTM( expression, ... )                                          \
        do {                                                                    \
        PRAGMA_DISABLE_PUSH_MSVC(4127)                                          \
            if ( !( expression ) )                                              \
            {                                                                   \
                if ( AssertHandler::FailureM( #expression, __FILE__, __LINE__, __VA_ARGS__ ) )\
                {                                                               \
                    BREAK_IN_DEBUGGER;                                              \
                }                                                               \
            }                                                                   \
        } while ( false )                                                       \
        PRAGMA_DISABLE_POP_MSVC

    // assert result of code, but still execute code when asserts are disabled
    #define VERIFY( code ) ASSERT( code )

    class AssertHandler
    {
    public:
        static void SetThrowOnAssert( bool throwOnAssert )
        {
            s_ThrowOnAssert = throwOnAssert;
        }
        static bool Failure( const char * message,
                             const char * file,
                             const int line ) NORETURN_CLANG_ANALYZER;
        static bool FailureM( const char * message,
                              const char * file,
                              const int line,
                              const char * msgFormat,
                              ... ) FORMAT_STRING( 4, 5 ) NORETURN_CLANG_ANALYZER;

        static bool s_ThrowOnAssert;
    };

// RELEASE
//------------------------------------------------------------------------------
#elif defined ( RELEASE )
    #define ASSERT( expression )            \
        do {                                \
        PRAGMA_DISABLE_PUSH_MSVC(4127)      \
        } while ( false )                   \
        PRAGMA_DISABLE_POP_MSVC

    #define VERIFY( code )                  \
        do {                                \
            if ( code ) {}                  \
        PRAGMA_DISABLE_PUSH_MSVC(4127)      \
        } while ( false )                   \
        PRAGMA_DISABLE_POP_MSVC
#else
    #error neither DEBUG nor RELEASE were defined
#endif

//------------------------------------------------------------------------------
