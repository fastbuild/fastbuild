// Types.h
//------------------------------------------------------------------------------
#pragma once

#if defined( __LINUX__ ) || defined( __OSX__ )
    #include <sys/types.h>
#endif

typedef unsigned char       uint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
#if defined( __WINDOWS__ ) || defined( __OSX__ )
    typedef unsigned long long  uint64_t;
    typedef signed long long    int64_t;
    #ifndef PRIi64
        #define PRIi64 "lli"
    #endif
    #ifndef PRIu64
        #define PRIu64 "llu"
    #endif
    #ifndef PRIx64
        #define PRIx64 "llx"
    #endif
    #ifndef PRIX64
        #define PRIX64 "llX"
    #endif
#else
    typedef unsigned long int   uint64_t;
    typedef signed long int     int64_t;
    #ifndef PRIi64
        #define PRIi64 "li"
    #endif
    #ifndef PRIu64
        #define PRIu64 "lu"
    #endif
    #ifndef PRIx64
        #define PRIx64 "lx"
    #endif
    #ifndef PRIX64
        #define PRIX64 "lX"
    #endif
#endif

#define KILOBYTE (1024)
#define MEGABYTE (1024 * 1024)

#if defined( __WINDOWS__ )
    #define THREAD_LOCAL __declspec( thread )
#else
    #define THREAD_LOCAL __thread
#endif

#if defined( __WINDOWS__ )
    #define NO_INLINE __declspec( noinline )
    #define FORCE_INLINE __forceinline
#elif defined( __APPLE__ ) || defined( __LINUX__ )
    #define NO_INLINE inline __attribute__((noinline))
    #define FORCE_INLINE inline __attribute__((always_inline))
#endif

#if defined( __clang__ )
    #define NORETURN_CLANG_ANALYZER __attribute__((analyzer_noreturn))
#else
    #define NORETURN_CLANG_ANALYZER
#endif

#if defined( __APPLE__ ) || defined( __LINUX__ )
    #include <stddef.h> // needed for size_t
#endif

#ifndef intptr_t
    #if defined( WIN64 )
        typedef int64_t     intptr_t;
        typedef uint64_t    uintptr_t;
    #elif defined( WIN32 )
        typedef int32_t     intptr_t;
        typedef uint32_t    uintptr_t;
    #endif
#endif
#ifndef uintptr_t
    #if defined( __LINUX__ )
        #if defined( __X64__ ) || defined( __ARM64__ )
            typedef uint64_t    uintptr_t;
        #else
            typedef uint32_t    uintptr_t;
        #endif
    #endif
#endif
#ifndef size_t
    #if defined( WIN64 )
        typedef uint64_t    size_t;
    #elif defined( WIN32 )
        typedef uint32_t    size_t;
    #endif
#endif

// Versions of Visual Studio prior to 2017 don't manage noexcept properly
#if defined( _MSC_VER ) && ( _MSC_VER < 1910 ) && !defined( __clang__ )
    #define NOEXCEPT
#else
    #define NOEXCEPT noexcept
#endif

#ifndef LONGLONG
    typedef long long LONGLONG;
#endif

#if defined( __LINUX__ ) || defined( __APPLE__ )
    #define MemoryBarrier() __asm__ __volatile__("")
#endif

#if defined( __GNUC__ ) || defined( __clang__ ) // GCC or Clang
    #define FORMAT_STRING( fmt, args ) __attribute__((format(printf, fmt, args)))
#else
    #define FORMAT_STRING( fmt, args )
#endif

// Warning disabling
//------------------------------------------------------------------------------
#if defined( WIN32 ) || defined( WIN64 )
    #define PRAGMA_DISABLE_PUSH_MSVC( num ) __pragma(warning(push))         \
                                            __pragma(warning(disable:num))
    #define PRAGMA_DISABLE_POP_MSVC         __pragma(warning(pop))
#else
    // Do nothing on non-msvc platforms
    #define PRAGMA_DISABLE_PUSH_MSVC( num )
    #define PRAGMA_DISABLE_POP_MSVC
#endif
#if defined( __clang__ )
    #define PRAGMA_DISABLE_PUSH_CLANG( w )  __pragma( clang diagnostic push ) \
                                            __pragma( clang diagnostic ignored w )
    #define PRAGMA_DISABLE_POP_CLANG        __pragma( clang diagnostic pop )
#else
    #define PRAGMA_DISABLE_PUSH_CLANG( w )
    #define PRAGMA_DISABLE_POP_CLANG
#endif
#if defined( __WINDOWS__ ) && defined( __clang__ )
    #define PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( w ) PRAGMA_DISABLE_PUSH_CLANG( w )
    #define PRAGMA_DISABLE_POP_CLANG_WINDOWS PRAGMA_DISABLE_POP_CLANG
#else
    #define PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( w )
    #define PRAGMA_DISABLE_POP_CLANG_WINDOWS
#endif

//------------------------------------------------------------------------------
