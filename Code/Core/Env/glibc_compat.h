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
__asm__( ".symver dlclose,dlclose@GLIBC_2.2.5" );
__asm__( ".symver dlerror,dlerror@GLIBC_2.2.5" );
__asm__( ".symver dlopen,dlopen@GLIBC_2.2.5" );
__asm__( ".symver dlsym,dlsym@GLIBC_2.2.5" );
__asm__( ".symver memcpy,memcpy@GLIBC_2.2.5" );
__asm__( ".symver posix_spawn,posix_spawn@GLIBC_2.2.5" );
__asm__( ".symver pthread_attr_setstacksize,pthread_attr_setstacksize@GLIBC_2.2.5" );
__asm__( ".symver pthread_create,pthread_create@GLIBC_2.2.5" );
__asm__( ".symver pthread_detach,pthread_detach@GLIBC_2.2.5" );
__asm__( ".symver pthread_join,pthread_join@GLIBC_2.2.5" );
__asm__( ".symver pthread_mutexattr_init,pthread_mutexattr_init@GLIBC_2.2.5" );
__asm__( ".symver pthread_mutexattr_settype,pthread_mutexattr_settype@GLIBC_2.2.5" );
__asm__( ".symver pthread_mutex_trylock,pthread_mutex_trylock@GLIBC_2.2.5" );
__asm__( ".symver pthread_timedjoin_np,pthread_timedjoin_np@GLIBC_2.3.3" );
__asm__( ".symver sem_destroy,sem_destroy@GLIBC_2.2.5" );
__asm__( ".symver sem_init,sem_init@GLIBC_2.2.5" );
__asm__( ".symver sem_post,sem_post@GLIBC_2.2.5" );
__asm__( ".symver sem_timedwait,sem_timedwait@GLIBC_2.2.5" );
__asm__( ".symver sem_wait,sem_wait@GLIBC_2.2.5" );
__asm__( ".symver shm_open,shm_open@GLIBC_2.2.5" );
__asm__( ".symver shm_unlink,shm_unlink@GLIBC_2.2.5" );
__asm__( ".symver __isoc99_vsscanf,vsscanf@GLIBC_2.2.5" );

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
extern "C"
{
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
