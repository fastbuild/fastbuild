// TestThread.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Process/Thread.h"

// TestThread
//------------------------------------------------------------------------------
class TestThread : public TestGroup
{
private:
    DECLARE_TESTS

    // Tests
    void Unused() const;
    void StartAndJoin() const;

    // Helpers
    static uint32_t ThreadFunc( void * userData )
    {
        const size_t returnValue = reinterpret_cast<size_t>( userData );
        return static_cast<uint32_t>( returnValue );
    }
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestThread )
    REGISTER_TEST( Unused )
    REGISTER_TEST( StartAndJoin )
REGISTER_TESTS_END

// Unused
//------------------------------------------------------------------------------
void TestThread::Unused() const
{
    // A thread object never used to create a thread
    Thread t;
}

// StartAndJoin
//------------------------------------------------------------------------------
void TestThread::StartAndJoin() const
{
    // Test thread will return user data as return value
    const uint32_t userData = 99;

    // Start thread
    Thread t;
    t.Start( ThreadFunc,
             "StartAndJoin",
             reinterpret_cast<void *>( static_cast<size_t>( userData ) ) );

    // Join and check result
    const uint32_t result = t.Join();
    TEST_ASSERT( result == userData );
}

//------------------------------------------------------------------------------
