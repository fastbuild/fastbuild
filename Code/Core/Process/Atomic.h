// Atomic.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include <intrin.h>
#endif

// bool
//------------------------------------------------------------------------------
inline bool AtomicLoadRelaxed( const volatile bool * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        return *x;
    #else
        #error Unknown compiler
    #endif
}
inline bool AtomicLoadAcquire( const volatile bool * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_ACQUIRE );
    #elif defined( _MSC_VER )
        const bool value = *x;
        _ReadWriteBarrier();
        return value;
    #else
        #error Unknown compiler
    #endif
}
inline void AtomicStoreRelaxed( volatile bool * x, bool value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        *x = value;
    #else
        #error Unknown compiler
    #endif
}
inline void AtomicStoreRelease( volatile bool * x, bool value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELEASE );
    #elif defined( _MSC_VER )
        _ReadWriteBarrier();
        *x = value;
    #else
        #error Unknown compiler
    #endif
}

// Pointers
//------------------------------------------------------------------------------
template < class T >
inline T * AtomicLoadRelaxed( T * const volatile * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        return *x;
    #else
        #error Unknown compiler
    #endif
}
template < class T >
inline T * AtomicLoadAcquire( T * const volatile * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_ACQUIRE );
    #elif defined( _MSC_VER )
        T * value = *x;
        _ReadWriteBarrier();
        return value;
    #else
        #error Unknown compiler
    #endif
}
template < class T >
inline void AtomicStoreRelaxed( T * volatile * x, T * value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        *x = value;
    #else
        #error Unknown compiler
    #endif
}
template < class T >
inline void AtomicStoreRelease( T * volatile * x, T * value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELEASE );
    #elif defined( _MSC_VER )
        _ReadWriteBarrier();
        *x = value;
    #else
        #error Unknown compiler
    #endif
}

// 32bit
//------------------------------------------------------------------------------
inline int32_t AtomicInc32( volatile int32_t * i )
{
    #if defined( __WINDOWS__ )
        return (int32_t)InterlockedIncrement( (volatile uint32_t *)i );
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
        return (int32_t)InterlockedDecrement( (volatile uint32_t *)i );
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
inline int32_t AtomicAdd32( volatile int32_t * i, int32_t value )
{
    #if defined( __WINDOWS__ )
        return InterlockedAdd( (volatile long *)i, value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, value );
    #endif
}
inline uint32_t AtomicAddU32( volatile uint32_t * i, int32_t value )
{
    #if defined( __WINDOWS__ )
        return (uint32_t)InterlockedAdd( reinterpret_cast<volatile long *>( i ), value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, value );
    #endif
}
inline int32_t AtomicSub32( volatile int32_t * i, int32_t value )
{
    #if defined( __WINDOWS__ )
        return InterlockedAdd( (volatile long *)i, -value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, value );
    #endif
}
inline uint32_t AtomicSubU32( volatile uint32_t * i, int32_t value )
{
    #if defined( __WINDOWS__ )
        return (uint32_t)InterlockedAdd( reinterpret_cast<volatile long*>( i ), -value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, value );
    #endif
}

inline int32_t AtomicLoadRelaxed( const volatile int32_t * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        return *x;
    #else
        #error Unknown compiler
    #endif
}
inline uint32_t AtomicLoadRelaxed( const volatile uint32_t * x )
{
    return static_cast< uint32_t >( AtomicLoadRelaxed( reinterpret_cast< const volatile int32_t * >( x ) ) );
}
inline int32_t AtomicLoadAcquire( const volatile int32_t * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_ACQUIRE );
    #elif defined( _MSC_VER )
        const int32_t value = *x;
        _ReadWriteBarrier();
        return value;
    #else
        #error Unknown compiler
    #endif
}
inline uint32_t AtomicLoadAcquire( const volatile uint32_t * x )
{
    return static_cast< uint32_t >( AtomicLoadAcquire( reinterpret_cast< const volatile int32_t * >( x ) ) );
}
inline void AtomicStoreRelaxed( volatile int32_t * x, int32_t value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        *x = value;
    #else
        #error Unknown compiler
    #endif
}
inline void AtomicStoreRelaxed( volatile uint32_t * x, uint32_t value )
{
    AtomicStoreRelaxed( reinterpret_cast< volatile int32_t * >( x ), static_cast< int32_t >( value ) );
}
inline void AtomicStoreRelease( volatile int32_t * x, int32_t value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELEASE );
    #elif defined( _MSC_VER )
        _ReadWriteBarrier();
        *x = value;
    #else
        #error Unknown compiler
    #endif
}
inline void AtomicStoreRelease( volatile uint32_t * x, uint32_t value )
{
    AtomicStoreRelease( reinterpret_cast< volatile int32_t * >( x ), static_cast< int32_t >( value ) );
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
        return (uint64_t)InterlockedIncrement64( (volatile int64_t *)i );
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
        return (uint64_t)InterlockedDecrement64( (volatile int64_t *)i );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, 1 );
    #endif
}
inline int64_t AtomicAdd64( volatile int64_t * i, int64_t value )
{
    #if defined( __WINDOWS__ )
        return InterlockedAdd64( (volatile LONG64 *)i, value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, value );
    #endif
}
inline uint64_t AtomicAddU64( volatile uint64_t * i, int64_t value )
{
    #if defined( __WINDOWS__ )
        return (uint64_t)InterlockedAdd64( reinterpret_cast<volatile LONG64 *>( i ), value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_add_and_fetch( i, value );
    #endif
}
inline int64_t AtomicSub64( volatile int64_t * i, int64_t value )
{
    #if defined( __WINDOWS__ )
        return InterlockedAdd64( (volatile LONG64 *)i, -value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, value );
    #endif
}
inline uint64_t AtomicSubU64( volatile uint64_t * i, int64_t value )
{
    #if defined( __WINDOWS__ )
        return (uint64_t)InterlockedAdd64( reinterpret_cast<volatile LONG64*>( i ), -value );
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        return __sync_sub_and_fetch( i, value );
    #endif
}

inline int64_t AtomicLoadRelaxed( const volatile int64_t * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        return *x;
    #else
        #error Unknown compiler
    #endif
}
inline uint64_t AtomicLoadRelaxed( const volatile uint64_t * x )
{
    return static_cast< uint64_t >( AtomicLoadRelaxed( reinterpret_cast< const volatile int64_t * >( x ) ) );
}
inline int64_t AtomicLoadAcquire( const volatile int64_t * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_ACQUIRE );
    #elif defined( _MSC_VER )
        const int64_t value = *x;
        _ReadWriteBarrier();
        return value;
    #else
        #error Unknown compiler
    #endif
}
inline uint64_t AtomicLoadAcquire( const volatile uint64_t * x )
{
    return static_cast< uint64_t >( AtomicLoadAcquire( reinterpret_cast< const volatile int64_t * >( x ) ) );
}
inline void AtomicStoreRelaxed( volatile int64_t * x, int64_t value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        *x = value;
    #else
        #error Unknown compiler
    #endif
}
inline void AtomicStoreRelaxed( volatile uint64_t * x, uint64_t value )
{
    AtomicStoreRelaxed( reinterpret_cast< volatile int64_t * >( x ), static_cast< int64_t >( value ) );
}
inline void AtomicStoreRelease( volatile int64_t * x, int64_t value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELEASE );
    #elif defined( _MSC_VER )
        _ReadWriteBarrier();
        *x = value;
    #else
        #error Unknown compiler
    #endif
}
inline void AtomicStoreRelease( volatile uint64_t * x, uint64_t value )
{
    AtomicStoreRelease( reinterpret_cast< volatile int64_t * >( x ), static_cast< int64_t >( value ) );
}

//------------------------------------------------------------------------------
