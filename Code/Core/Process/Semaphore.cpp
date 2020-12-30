// Event
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Semaphore.h"

// Core
#include "Core/Env/Assert.h"

#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif
#if defined( __LINUX__ )
    #include <errno.h>
    #include <time.h>
#endif
#if defined( __OSX__ )
    #include <dispatch/dispatch.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    Semaphore::Semaphore( uint32_t maxCount )
    {
        ASSERT( maxCount > 0 ); // Must be possible to signal at least once
        m_Semaphore = CreateSemaphore( nullptr, 0, (LONG)maxCount, nullptr );
        ASSERT( m_Semaphore );
        m_HasMaxCount = true;
    }
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
Semaphore::Semaphore()
{
    #if defined( __WINDOWS__ )
        m_Semaphore = CreateSemaphore( nullptr, 0, 0x7FFFFFFF, nullptr );
        ASSERT( m_Semaphore );
        m_HasMaxCount = false;
    #elif defined( __APPLE__ )
        m_Semaphore = dispatch_semaphore_create( 0 );
        ASSERT( m_Semaphore );
    #elif defined( __LINUX__ )
        VERIFY( sem_init( &m_Semaphore, 0, 0 ) == 0 );
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Semaphore::~Semaphore()
{
    #if defined( __WINDOWS__ )
        VERIFY( CloseHandle( m_Semaphore ) );
    #elif defined( __APPLE__ )
        dispatch_release( m_Semaphore );
    #elif defined( __LINUX__ )
        VERIFY( sem_destroy( &m_Semaphore ) == 0 );
    #endif
}

// Signal
//------------------------------------------------------------------------------
void Semaphore::Signal()
{
    #if defined( __WINDOWS__ )
        if ( ReleaseSemaphore( m_Semaphore, 1, nullptr ) == FALSE )
        {
            // If the Semaphore has a max count, we may have hit it
            // Any other failure is an error
            ASSERT( GetLastError() == ERROR_TOO_MANY_POSTS );
        }
    #elif defined( __APPLE__ )
        dispatch_semaphore_signal( m_Semaphore );
    #elif defined( __LINUX__ )
        VERIFY( sem_post( &m_Semaphore ) == 0 );
    #endif
}

// Signal
//------------------------------------------------------------------------------
void Semaphore::Signal( uint32_t num )
{
    ASSERT( num ); // not valid to call with 0

    #if defined( __WINDOWS__ )
        // On Windows, we can signal all the events in one call, but only if we're
        // not using the "max count" feature. This is because ReleaseSemaphore is
        // all or nothing, but what we want is for it to signal upto maxCount
        if ( m_HasMaxCount == false )
        {
            VERIFY( ReleaseSemaphore( m_Semaphore, (LONG)num, nullptr ) == TRUE );
            return;
        }
    #endif

    for ( size_t i = 0; i < num; ++i )
    {
        Signal();
    }
}

// Wait
//------------------------------------------------------------------------------
void Semaphore::Wait()
{
    #if defined( __WINDOWS__ )
        VERIFY( WaitForSingleObject( m_Semaphore, INFINITE ) == WAIT_OBJECT_0 );
    #elif defined( __APPLE__ )
        dispatch_semaphore_wait( m_Semaphore, DISPATCH_TIME_FOREVER );
    #elif defined( __LINUX__ )
        VERIFY( sem_wait( &m_Semaphore ) == 0 );
    #endif
}

// Wait
//------------------------------------------------------------------------------
bool Semaphore::Wait( uint32_t timeoutMS )
{
    #if defined( __WINDOWS__ )
        const DWORD result = WaitForSingleObject( m_Semaphore, timeoutMS );
        ASSERT( ( result == WAIT_OBJECT_0 ) || ( result == WAIT_TIMEOUT ) );
        return ( result == WAIT_OBJECT_0 );
    #elif defined( __APPLE__ )
        const intptr_t result = dispatch_semaphore_wait( m_Semaphore, dispatch_time( DISPATCH_TIME_NOW, timeoutMS * 1000000 ) );
        return ( result == 0 ); // 0 means signalled, non-zero means timeout was hit
    #elif defined( __LINUX__ )
        struct timespec ts;
        VERIFY( clock_gettime( CLOCK_REALTIME, &ts ) == 0 );
        uint64_t totalNS = (uint64_t)ts.tv_sec * (uint64_t)1000000000;
        totalNS += (uint64_t)ts.tv_nsec;
        totalNS += (uint64_t)timeoutMS * 1000000;
        ts.tv_sec = totalNS / 1000000000;
        ts.tv_nsec = totalNS % 1000000000;
        const int32_t result = sem_timedwait( &m_Semaphore, &ts );
        if ( result != 0 )
        {
            // TODO:C checking EINTR seems suspect here
            ASSERT( ( errno == ETIMEDOUT ) || ( errno == EINTR ) ); // anything else is a usage error?
        }
        return ( result == 0 ); // 0 means signalled, non-zero means timeout was hit
    #endif
}

//------------------------------------------------------------------------------
