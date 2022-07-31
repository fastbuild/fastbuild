// glibc_compat
//
// Target older versions of glibc for better Linux distro compatibility
//------------------------------------------------------------------------------
#pragma once

// Avoid "Fortify Source" checks
//------------------------------------------------------------------------------
#undef _FORTIFY_SOURCE

// Use older implementations of glibc functions
//------------------------------------------------------------------------------
__asm__( ".symver dlclose,dlclose@GLIBC_2.2.5" );
__asm__( ".symver dlerror,dlerror@GLIBC_2.2.5" );
__asm__( ".symver dlopen,dlopen@GLIBC_2.2.5" );
__asm__( ".symver dlsym,dlsym@GLIBC_2.2.5" );
__asm__( ".symver memcpy,memcpy@GLIBC_2.2.5" );
__asm__( ".symver pthread_attr_setstacksize,pthread_attr_setstacksize@GLIBC_2.2.5" );
__asm__( ".symver pthread_create,pthread_create@GLIBC_2.2.5" );
__asm__( ".symver pthread_detach,pthread_detach@GLIBC_2.2.5" );
__asm__( ".symver pthread_join,pthread_join@GLIBC_2.2.5" );
__asm__( ".symver pthread_mutexattr_init,pthread_mutexattr_init@GLIBC_2.2.5" );
__asm__( ".symver pthread_mutexattr_settype,pthread_mutexattr_settype@GLIBC_2.2.5" );
__asm__( ".symver pthread_timedjoin_np,pthread_timedjoin_np@GLIBC_2.3.3" );
__asm__( ".symver sem_destroy,sem_destroy@GLIBC_2.2.5" );
__asm__( ".symver sem_init,sem_init@GLIBC_2.2.5" );
__asm__( ".symver sem_post,sem_post@GLIBC_2.2.5" );
__asm__( ".symver sem_timedwait,sem_timedwait@GLIBC_2.2.5" );
__asm__( ".symver sem_wait,sem_wait@GLIBC_2.2.5" );
__asm__( ".symver shm_open,shm_open@GLIBC_2.2.5" );
__asm__( ".symver shm_unlink,shm_unlink@GLIBC_2.2.5" );

//------------------------------------------------------------------------------
