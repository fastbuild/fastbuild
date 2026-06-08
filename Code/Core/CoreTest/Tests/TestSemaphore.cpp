// TestSemaphore.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include <Core/Process/Semaphore.h>
#include <Core/Process/Thread.h>

//------------------------------------------------------------------------------
TEST_GROUP( TestSemaphore, TestGroupTest )
{
public:
    // Internal helpers
    static uint32_t WaitForSignal_Thread( void * userData );
};

//------------------------------------------------------------------------------
TEST_CASE( TestSemaphore, CreateDestroy )
{
    Semaphore s;
}

//------------------------------------------------------------------------------
TEST_CASE( TestSemaphore, WaitForSignal )
{
    Semaphore s;

    // Create a thread which will signal the Semaphore
    Thread t;
    t.Start( WaitForSignal_Thread, "Test::WaitForSignal", &s );

    // Wait or the expected signal count
    for ( size_t i = 0; i < 100; ++i )
    {
        s.Wait();
    }

    // Cleanup thread
    t.Join();
}

//------------------------------------------------------------------------------
/*static*/ uint32_t TestSemaphore::WaitForSignal_Thread( void * userData )
{
    Semaphore * s = static_cast<Semaphore *>( userData );
    s->Signal( 100 );
    return 0;
}

//------------------------------------------------------------------------------
TEST_CASE( TestSemaphore, WaitTimeout )
{
    const Timer t;

    Semaphore s;

    // Check for signalled
    {
        s.Signal();
        const bool signalled = s.Wait( 1 ); // Wait 1ms
        TEST_ASSERT( signalled == true ); // Should be signalled (should not time out)
    }

    // Check for timeout
    {
        const bool signalled = s.Wait( 50 ); // Wait 50ms
        TEST_ASSERT( signalled == false ); // Should not be signalled (should time out)
    }

    // ensure some sensible time has elapsed
    TEST_ASSERT( t.GetElapsed() > 0.025f ); // 25ms (allow wide margin of error)
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
TEST_CASE( TestSemaphore, MaxCount )
{
    // Only Windows supports a signal count limit for Semaphores

    // Create semaphore with a max count
    Semaphore s( 1 );

    // Signal with individual calls
    {
        // Signal more than the max count
        s.Signal(); // This should signal
        s.Signal(); // This should gracefully fail
        TEST_ASSERT( s.Wait( 1 ) == true ); // First wait should see signalled state
        TEST_ASSERT( s.Wait( 1 ) == false ); // Second wait should time out
    }

    // Signal with single call
    {
        // Signal more than the max count
        s.Signal( 2 ); // This should signal once
        TEST_ASSERT( s.Wait( 1 ) == true ); // First wait should see signalled state
        TEST_ASSERT( s.Wait( 1 ) == false ); // Second wait should time out
    }
}
#endif

//------------------------------------------------------------------------------
