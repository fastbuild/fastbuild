// TestSemaphore.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include <Core/Process/Semaphore.h>
#include <Core/Process/Thread.h>

// TestSemaphore
//------------------------------------------------------------------------------
class TestSemaphore : public TestGroup
{
private:
    DECLARE_TESTS

    void CreateDestroy() const;
    void WaitForSignal() const;
    void WaitTimeout() const;
    #if defined( __WINDOWS__ )
        void MaxCount() const;
    #endif

    // Internal helpers
    static uint32_t WaitForSignal_Thread( void * userData );
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestSemaphore )
    REGISTER_TEST( CreateDestroy )
    REGISTER_TEST( WaitForSignal )
    REGISTER_TEST( WaitTimeout )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( MaxCount )
    #endif
REGISTER_TESTS_END

// CreateDestroy
//------------------------------------------------------------------------------
void TestSemaphore::CreateDestroy() const
{
    Semaphore s;
}

// WaitForSignal
//------------------------------------------------------------------------------
void TestSemaphore::WaitForSignal() const
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

// WaitForSignal_Thread
//------------------------------------------------------------------------------
/*static*/ uint32_t TestSemaphore::WaitForSignal_Thread( void * userData )
{
    Semaphore * s = static_cast< Semaphore * >( userData );
    s->Signal( 100 );
    return 0;
}

// WaitTimeout
//------------------------------------------------------------------------------
void TestSemaphore::WaitTimeout() const
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

// MaxCount
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestSemaphore::MaxCount() const
    {
        // Only Windows supports a signal count limit for Semaphores

        // Create semaphore with a max count
        Semaphore s( 1 );

        // Signal with individual calls
        {
            // Signal more than the max count
            s.Signal(); // This should signal
            s.Signal(); // This should gracefully fail
            TEST_ASSERT( s.Wait( 1 ) == true );    // First wait should see signalled state
            TEST_ASSERT( s.Wait( 1 ) == false );   // Second wait should time out
        }

        // Signal with single call
        {
            // Signal more than the max count
            s.Signal( 2 ); // This should signal once
            TEST_ASSERT( s.Wait( 1 ) == true );    // First wait should see signalled state
            TEST_ASSERT( s.Wait( 1 ) == false );   // Second wait should time out
        }
    }
#endif

//------------------------------------------------------------------------------
