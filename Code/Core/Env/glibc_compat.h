// glibc_compat
//
// Target older versions of glibc for better Linux distro compatibility
//------------------------------------------------------------------------------
#pragma once

// Avoid "Fortify Source" checks
//------------------------------------------------------------------------------
#if defined( _FORTIFY_SOURCE )
    #undef _FORTIFY_SOURCE
#endif

// Use older implementations of glibc functions
//------------------------------------------------------------------------------
__asm__( ".symver clock_gettime,clock_gettime@GLIBC_2.2.5" );
__asm__( ".symver memcpy,memcpy@GLIBC_2.2.5" );

//------------------------------------------------------------------------------

// Use stat,lstat,fstat implementations from older versions of glibc for better Linux distro compatibility
//------------------------------------------------------------------------------
#include <sys/stat.h>

// In glibc 2.33 implementation of various stat functions was changed - usage of proxy functions (__xstat, etc.) was removed.
// As a result code built with glibc 2.33 headers links to stat@GLIBC_2.33 instead of __xstat@GLIBC_2.2.5.
// But in this case we can't use the __asm__(".symver") trick to use old versions of those functions,
// because there are no compatible versions - those proxy functions have a different signature.
// Thus we must use a more involved hack to force usage of old versions to avoid depending on a new glibc version.

#if defined( __GLIBC__ ) && ( __GLIBC__ * 1000 + __GLIBC_MINOR__ ) >= 2033

    #ifdef __cplusplus
        extern "C" {
    #endif

    int __xstat( int ver, const char * path, struct stat * buf );
    __extern_inline int stat( const char * path, struct stat * buf )
    {
        return __xstat( 0 /*_STAT_VER*/, path, buf );
    }

    int __lxstat( int ver, const char * path, struct stat * buf );
    __extern_inline int lstat( const char * path, struct stat * buf )
    {
        return __lxstat( 0 /*_STAT_VER*/, path, buf );
    }

    int __fxstat( int ver, int fildes, struct stat * buf );
    __extern_inline int fstat( int fildes, struct stat * buf )
    {
        return __fxstat( 0 /*_STAT_VER*/, fildes, buf );
    }

    #ifdef __cplusplus
        }
    #endif

#endif

//------------------------------------------------------------------------------
