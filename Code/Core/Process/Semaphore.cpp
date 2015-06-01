// Event
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "Semaphore.h"

// Core
#include "Core/Env/Assert.h"

#if defined( __WINDOWS__ )
    #include <windows.h>
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
Semaphore::Semaphore()
{ 
    #if defined( __WINDOWS__ )
        m_Semaphore = CreateSemaphore( nullptr, 0, 0x7FFFFFFF, nullptr );
        ASSERT( m_Semaphore );
    #elif defined( __APPLE__ )
        m_Semaphore = dispatch_semaphore_create(0);
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
        VERIFY( ReleaseSemaphore( m_Semaphore, 1, nullptr ) );
    #elif defined( __APPLE__ )
        dispatch_semaphore_signal( m_Semaphore );
    #elif defined( __LINUX__ )
        VERIFY( sem_post( &m_Semaphore ) == 0 );
    #endif
}

// Wait
//------------------------------------------------------------------------------
void Semaphore::Wait( uint32_t timeoutMS )
{
    if ( timeoutMS == 0 )
    {
        // Wait forever
        #if defined( __WINDOWS__ )
            VERIFY( WaitForSingleObject( m_Semaphore, INFINITE ) == WAIT_OBJECT_0 );        
        #elif defined( __APPLE__ )
            dispatch_semaphore_wait( m_Semaphore, DISPATCH_TIME_FOREVER );
        #elif defined( __LINUX__ )
            VERIFY( sem_wait( &m_Semaphore ) == 0 );
        #endif
    }
    else
    {
        // Wait with timeout
        #if defined( __WINDOWS__ )
            DWORD result = WaitForSingleObject( m_Semaphore, timeoutMS );
            ASSERT( ( result == WAIT_OBJECT_0 ) || ( result == WAIT_TIMEOUT ) );
            (void)result;
        #elif defined( __APPLE__ )
            dispatch_semaphore_wait( m_Semaphore, dispatch_time( DISPATCH_TIME_NOW, timeoutMS * 1000000 ) );
        #elif defined( __LINUX__ )
            struct timespec ts;
            VERIFY( clock_gettime( CLOCK_REALTIME, &ts ) == 0 );
            uint64_t totalNS = (uint64_t)ts.tv_sec * (uint64_t)1000000000;
            totalNS += (uint64_t)ts.tv_nsec;
            totalNS += (uint64_t)timeoutMS * 1000000;
            ts.tv_sec = totalNS / 1000000000;
            ts.tv_nsec = totalNS % 1000000000;
            if ( sem_timedwait( &m_Semaphore, &ts ) != 0 )
            {
                ASSERT( errno == ETIMEDOUT ); // anything else is a usage error?
            }
        #endif
    }
}

//------------------------------------------------------------------------------
