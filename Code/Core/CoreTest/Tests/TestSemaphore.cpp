// TestSempahore.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include <Core/Process/Thread.h>
#include <Core/Process/Semaphore.h>

// TestSemaphore
//------------------------------------------------------------------------------
class TestSemaphore : public UnitTest
{
private:
	DECLARE_TESTS

	void CreateDestroy() const;
	void WaitForSignal() const;
    void WaitTimeout() const;

    // Internal helpers
    static uint32_t WaitForSignal_Thread( void * userData );
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestSemaphore )
	REGISTER_TEST( CreateDestroy )
	REGISTER_TEST( WaitForSignal )
    REGISTER_TEST( WaitTimeout )
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
    Thread::ThreadHandle h = Thread::CreateThread( WaitForSignal_Thread, "Test::WaitForSignal", ( 32 * KILOBYTE ), &s );

    // Wait or the expected signal count
    for ( size_t i=0; i<100; ++i )
    {
        s.Wait();
    }

    // Cleanup thread
    bool timedOut;
    Thread::WaitForThread( h, 1000, timedOut );
    TEST_ASSERT( timedOut == false );
    Thread::CloseHandle( h );
}

// WaitForSignal_Thread
//------------------------------------------------------------------------------
/*static*/ uint32_t TestSemaphore::WaitForSignal_Thread( void * userData )
{
    Semaphore * s = reinterpret_cast< Semaphore * >( userData );
    for ( size_t i=0; i<100; ++i )
    {
        s->Signal();
    }
    return 0;
}

// WaitTimeout
//------------------------------------------------------------------------------
void TestSemaphore::WaitTimeout() const
{
    Timer t;
    
    Semaphore s;
    s.Wait( 50 ); // wait 50ms
    
    // ensure some sensible time has elapsed
    ASSERT( t.GetElapsed() > 0.025f ); // 25ms (allow wide margin of error)
}

//------------------------------------------------------------------------------
