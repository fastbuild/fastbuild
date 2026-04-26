// TestThread.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Process/Thread.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestThread, TestGroupTest )
{
public:
    // Helpers
    static uint32_t ThreadFunc( void * userData )
    {
        const size_t returnValue = reinterpret_cast<size_t>( userData );
        return static_cast<uint32_t>( returnValue );
    }
};

//------------------------------------------------------------------------------
TEST_CASE( TestThread, Unused )
{
    // A thread object never used to create a thread
    Thread t;
    TEST_ASSERT( t.IsRunning() == false );
}

//------------------------------------------------------------------------------
TEST_CASE( TestThread, StartAndJoin )
{
    // Test thread will return user data as return value
    const uint32_t userData = 99;

    // Start thread
    Thread t;
    t.Start( ThreadFunc,
             "StartAndJoin",
             reinterpret_cast<void *>( static_cast<size_t>( userData ) ) );
    TEST_ASSERT( t.IsRunning() );

    // Join and check result
    const uint32_t result = t.Join();
    TEST_ASSERT( result == userData );
    TEST_ASSERT( t.IsRunning() == false );
}

//------------------------------------------------------------------------------
