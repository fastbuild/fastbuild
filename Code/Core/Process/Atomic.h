// Atomic.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#if defined( __WINDOWS__ )
    #include <windows.h>
#endif

// 32bit
//------------------------------------------------------------------------------
inline int32_t AtomicInc32( volatile int32_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedIncrement( (volatile uint32_t *)i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, 1 );
    #endif
}
inline uint32_t AtomicIncU32( volatile uint32_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedIncrement( i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, 1 );
    #endif
}
inline int32_t AtomicDec32( volatile int32_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedDecrement( (volatile uint32_t *)i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, 1 );
    #endif
}
inline uint32_t AtomicDecU32( volatile uint32_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedDecrement( i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, 1 );
    #endif
}

// 64bit
//------------------------------------------------------------------------------
inline int64_t AtomicInc64( volatile int64_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedIncrement64( i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, 1 );
    #endif
}
inline uint64_t AtomicIncU64( volatile uint64_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedIncrement64( (volatile int64_t *)i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, 1 );
    #endif
}
inline int64_t AtomicDec64( volatile int64_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedDecrement64( i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, 1 );
    #endif
}
inline uint64_t AtomicDecU64( volatile uint64_t * i )
{
    #if defined( __WINDOWS__ )
        return InterlockedDecrement64( (volatile int64_t *)i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, 1 );
    #endif
}

//------------------------------------------------------------------------------
