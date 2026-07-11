// TestThreadPool.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Process/Atomic.h"
#include "Core/Process/ThreadPool.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestThreadPool, TestGroupTest )
{
public:
    // Helpers
    static void Increment( void * userData )
    {
        Atomic<uint32_t> * u32 = static_cast<Atomic<uint32_t> *>( userData );
        u32->Increment();
    }
};

//------------------------------------------------------------------------------
TEST_CASE( TestThreadPool, Unused )
{
    // A thread pool never used
    ThreadPool threadPool( 4 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestThreadPool, SingleJob )
{
    Atomic<uint32_t> count;

    // Create
    ThreadPool * threadPool = FNEW( ThreadPool( 4 ) );

    // Enqueue jobs
    threadPool->EnqueueJob( Increment, &count );

    // Destroy
    FDELETE threadPool;

    TEST_ASSERT( count.Load() == 1 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestThreadPool, MultipleJobs )
{
    Atomic<uint32_t> count;

    // Create
    ThreadPool * threadPool = FNEW( ThreadPool( 4 ) );

    // Enqueue jobs
    const uint32_t numJobs = 1024;
    for ( uint32_t i = 0; i < numJobs; ++i )
    {
        threadPool->EnqueueJob( Increment, &count );
    }

    // Destroy
    FDELETE threadPool;

    TEST_ASSERT( count.Load() == numJobs );
}

//------------------------------------------------------------------------------
