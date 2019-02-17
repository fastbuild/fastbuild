// Mutex
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Mutex.h"

// Core
#include "Core/Env/Assert.h"

#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <pthread.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
Mutex::Mutex()
{
    #if defined( __WINDOWS__ )
        static_assert( sizeof( m_CriticalSection ) == sizeof( CRITICAL_SECTION ), "Unexpected sizeof(CRITICAL_SECTION)" );
        static_assert( __alignof( decltype( m_CriticalSection ) ) == __alignof( CRITICAL_SECTION ), "Unexpected __alignof(CRITICAL_SECTION)" );

        VERIFY( InitializeCriticalSectionAndSpinCount( (CRITICAL_SECTION *)&m_CriticalSection, 100 ) );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        pthread_mutexattr_t attributes;
        VERIFY( pthread_mutexattr_init( &attributes ) == 0 );
        pthread_mutexattr_settype( &attributes, PTHREAD_MUTEX_RECURSIVE );
        VERIFY( pthread_mutex_init( &m_Mutex, &attributes ) == 0 );
    #else
        #error Unknown platform
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Mutex::~Mutex()
{
    #if defined( __WINDOWS__ )
        DeleteCriticalSection( (CRITICAL_SECTION *)&m_CriticalSection );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        VERIFY( pthread_mutex_destroy( &m_Mutex ) == 0 );
    #else
        #error Unknown platform
    #endif
}

PRAGMA_DISABLE_PUSH_MSVC( 26135 ) // static analysis complains about missing annotation
// Lock
//------------------------------------------------------------------------------
void Mutex::Lock()
{
    #if defined( __WINDOWS__ )
        EnterCriticalSection( (CRITICAL_SECTION *)&m_CriticalSection );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        VERIFY( pthread_mutex_lock( &m_Mutex ) == 0 );
    #else
        #error Unknown platform
    #endif
}
// Unlock
//------------------------------------------------------------------------------
void Mutex::Unlock()
{
    #if defined( __WINDOWS__ )
        LeaveCriticalSection( (CRITICAL_SECTION *)&m_CriticalSection );
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        VERIFY( pthread_mutex_unlock( &m_Mutex ) == 0 );
    #else
        #error Unknown platform
    #endif
}
PRAGMA_DISABLE_POP_MSVC

//------------------------------------------------------------------------------
