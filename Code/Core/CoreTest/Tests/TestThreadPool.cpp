// TestThreadPool.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Process/Atomic.h"
#include "Core/Process/ThreadPool.h"

// TestThread
//------------------------------------------------------------------------------
class TestThreadPool : public TestGroup
{
private:
    DECLARE_TESTS

    // Tests
    void Unused() const;
    void SingleJob() const;
    void MultipleJobs() const;

    // Helpers
    static void Increment( void * userData )
    {
        Atomic<uint32_t> * u32 = static_cast<Atomic<uint32_t> *>( userData );
        u32->Increment();
    }
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestThreadPool )
    REGISTER_TEST( Unused )
    REGISTER_TEST( SingleJob )
    REGISTER_TEST( MultipleJobs )
REGISTER_TESTS_END

// Unused
//------------------------------------------------------------------------------
void TestThreadPool::Unused() const
{
    // A thread pool never used
    ThreadPool threadPool( 4 );
}

// SingleJob
//------------------------------------------------------------------------------
void TestThreadPool::SingleJob() const
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

// MultipleJobs
//------------------------------------------------------------------------------
void TestThreadPool::MultipleJobs() const
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
