// Atomic.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

#if defined( __WINDOWS__ )
    #include <intrin.h>
#endif

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #if !defined( _M_X64 )
        // If some future Windows platforms on another architecture is supported
        // the implementation will need to be updated (arm for example)
        #error Windows atomic ops have been imlpemented assuming an x86 memory model
    #endif
#endif

// Memory Barrier
//------------------------------------------------------------------------------
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #define MemoryBarrier() __asm__ __volatile__("")
#else
    #define MemoryBarrier __faststorefence
#endif

// Pointers
//------------------------------------------------------------------------------
template < class T >
[[nodiscard]] inline T * AtomicLoadRelaxed( T * const volatile * x )
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
[[nodiscard]] inline T * AtomicLoadAcquire( T * const volatile * x )
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

// Atomic Inc/Dec/Add/Sub
//------------------------------------------------------------------------------
#if defined( __GNUC__ ) || defined( __clang__ )
    #if defined( __WINDOWS__ )
        PRAGMA_DISABLE_PUSH_CLANG( "-Watomic-implicit-seq-cst" )
    #endif
    template<typename T> inline T AtomicInc( volatile T * x )
    {
        return __sync_add_and_fetch( x, 1 );
    }
    template<typename T> inline T AtomicDec( volatile T * x )
    {
        return __sync_sub_and_fetch( x, 1 );
    }
    template<typename T> inline T AtomicAdd( volatile T * x, T value )
    {
        return __sync_add_and_fetch( x, value );
    }
    template<typename T> inline T AtomicSub( volatile T * x, T value )
    {
        return __sync_sub_and_fetch( x, value );
    }
    #if defined( __WINDOWS__ )
        PRAGMA_DISABLE_POP_CLANG
    #endif
#elif defined( _MSC_VER )
    // MSVC is missing intrinsics for some sizes so we emulate them:
    inline char     XInterlockedIncrement8( volatile char * x )                 { return ( _InterlockedExchangeAdd8( x, 1 ) + 1 ); }
    inline char     XInterlockedDecrement8( volatile char * x )                 { return ( _InterlockedExchangeAdd8( x, -1 ) - 1 ); }
    inline char     XInterlockedAdd8( volatile char * x, char value )           { return ( _InterlockedExchangeAdd8( x, value ) + value ); }
    inline short    XInterlockedAdd16( volatile short * x, short value )        { return ( _InterlockedExchangeAdd16( x, value ) + value ); }

    // The MSVC compiler crashes if the following intrinsics are used:
    //              _interlockedadd
    //              _interlockedadd64
    // These alternative implementations avoid the crash.
    //       (see: https://developercommunity.visualstudio.com/t/Internal-compiler-error-using-_interlock/10061949)
    inline long     XInterlockedAdd32( volatile long * x, long value )          { return ( _InterlockedExchangeAdd( x, value ) + value ); }
    inline __int64  XInterlockedAdd64( volatile __int64 * x, __int64 value )    { return ( _InterlockedExchangeAdd64( x, value ) + value ); }

    // MSVC has inconsistent function names (suffixes and casing) so the macro below can be used to map
    // a consistent set of function names for each type
    #define IMPLEMENT_FUNCTIONS( T, U, INCFUNC, DECFUNC, ADDFUNC ) \
        inline T AtomicInc( volatile T * x ) \
        { \
            return static_cast<T>( INCFUNC( reinterpret_cast<volatile U *>( x ) ) ); \
        } \
        inline T AtomicDec( volatile T * x ) \
        { \
            return static_cast<T>( DECFUNC( reinterpret_cast<volatile U *>( x ) ) ); \
        } \
        inline T AtomicAdd( volatile T * x, T value ) \
        { \
            return static_cast<T>( ADDFUNC( reinterpret_cast<volatile U *>( x ), static_cast<U>( value ) ) ); \
        } \
        inline T AtomicSub( volatile T * x, T value ) \
        { \
            return static_cast<T>( ADDFUNC( reinterpret_cast<volatile U *>( x ), -static_cast<U>( value ) ) ); \
        }

    IMPLEMENT_FUNCTIONS( uint8_t,  char,    XInterlockedIncrement8,     XInterlockedDecrement8,     XInterlockedAdd8 );
    IMPLEMENT_FUNCTIONS( int8_t,   char,    XInterlockedIncrement8,     XInterlockedDecrement8,     XInterlockedAdd8 );
    IMPLEMENT_FUNCTIONS( uint16_t, short,   _InterlockedIncrement16,    _InterlockedDecrement16,    XInterlockedAdd16 );
    IMPLEMENT_FUNCTIONS( int16_t,  short,   _InterlockedIncrement16,    _InterlockedDecrement16,    XInterlockedAdd16 );
    IMPLEMENT_FUNCTIONS( uint32_t, long,    _InterlockedIncrement,      _InterlockedDecrement,      XInterlockedAdd32 );
    IMPLEMENT_FUNCTIONS( int32_t,  long,    _InterlockedIncrement,      _InterlockedDecrement,      XInterlockedAdd32 );
    IMPLEMENT_FUNCTIONS( uint64_t, __int64, _InterlockedIncrement64,    _InterlockedDecrement64,    XInterlockedAdd64 );
    IMPLEMENT_FUNCTIONS( int64_t,  __int64, _InterlockedIncrement64,    _InterlockedDecrement64,    XInterlockedAdd64 );

    #undef IMPLEMENT_FUNCTIONS
#endif

// AtomicLoadRelaxed
//------------------------------------------------------------------------------
template<typename T>
[[nodiscard]] inline T AtomicLoadRelaxed( const volatile T * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        return *x;
    #endif
}

// AtomicLoadAcquire
//------------------------------------------------------------------------------
template<typename T>
[[nodiscard]] inline T AtomicLoadAcquire( const volatile T * x )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        return __atomic_load_n( x, __ATOMIC_ACQUIRE );
    #elif defined( _MSC_VER )
        const T value = *x;
        _ReadWriteBarrier();
        return value;
    #endif
}

// AtomicStoreRelaxed
//------------------------------------------------------------------------------
template<typename T>
inline void AtomicStoreRelaxed( volatile T * x, T value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELAXED );
    #elif defined( _MSC_VER )
        *x = value;
    #endif
}

// AtomicStoreRelease
//------------------------------------------------------------------------------
template<typename T>
inline void AtomicStoreRelease( volatile T * x, T value )
{
    #if defined( __GNUC__ ) || defined( __clang__ )
        __atomic_store_n( x, value, __ATOMIC_RELEASE );
    #elif defined( _MSC_VER )
        _ReadWriteBarrier();
        *x = value;
    #endif
}

//------------------------------------------------------------------------------
template <class T>
class Atomic
{
public:
    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wzero-as-null-pointer-constant" ) // Allow zero to work with pointers here
    Atomic()
        : m_Value( static_cast<T>( 0 ) )
    {
    }
    PRAGMA_DISABLE_POP_CLANG_WINDOWS
    explicit Atomic( T initValue )
        : m_Value( initValue )
    {
    }

    // Store/Load
    void            Store( T newValue ) { AtomicStoreRelease( &m_Value, newValue ); }
    [[nodiscard]] T Load() const        { return AtomicLoadAcquire( &m_Value ); }

    // Increment (returns result)
    T               Increment()         { return AtomicInc( &m_Value ); }
    T               Decrement()         { return AtomicDec( &m_Value ); }

    // Add (returns result)
    T               Add( T value )      { return AtomicAdd( &m_Value, value ); }
    T               Sub( T value )      { return AtomicSub( &m_Value, value ); }

protected:
    volatile T m_Value;
};

//------------------------------------------------------------------------------
