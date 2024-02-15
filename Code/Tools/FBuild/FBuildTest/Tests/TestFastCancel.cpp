// TestFastCancel.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/Math/Conversions.h"
#include "Core/Process/SystemMutex.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// TestFastCancel
//------------------------------------------------------------------------------
class TestFastCancel : public FBuildTest
{
private:
    DECLARE_TESTS

    void Cancel() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestFastCancel )
    #if defined( __WINDOWS__ )
        // TODO:LINUX TODO:OSX - Fix and enable this test
        REGISTER_TEST( Cancel )
    #endif
REGISTER_TESTS_END

// CancelHelperThread
//------------------------------------------------------------------------------
static uint32_t CancelHelperThread( void * )
{
    const Timer t;

    // The Exec step in our test spawns a hierarchy of processes. When the leaf
    // process of the tree is created, it acquires the final mutex
    // "FASTBuildFastCancelTest4"
    //
    // If we can acquire that, the process has either not started or has failed in
    // an unexpected way.
    SystemMutex mutex4( "FASTBuildFastCancelTest4" );
    uint32_t sleepTimeMS = 1;
    while ( mutex4.TryLock() == true )
    {
        // unlock so child can acquire
        mutex4.Unlock();
    
        // Wait before trying again, waiting longer each time up to a limit
        Thread::Sleep( sleepTimeMS );
        sleepTimeMS = Math::Min<uint32_t>( ( sleepTimeMS * 2 ), 128 );
        TEST_ASSERT( t.GetElapsed() < 5.0f ); // Ensure test doesn't hang if broken
    }

    // Once we are here, all child processes have acquired locks and will wait forever
    // so we terminate them to test cancellation
    FBuild::Get().AbortBuild();
    return 0;
}

// Cancel
//------------------------------------------------------------------------------
void TestFastCancel::Cancel() const
{
    // Ensure a hierarchy of processes can be correctly aborted
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestFastCancel/Cancel/fbuild.bff";
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well

    // Init
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Create thread that will abort build once all processes are spawned
    Thread thread;
    thread.Start( CancelHelperThread );

    // Start build and check it was aborted
    TEST_ASSERT( fBuild.Build( "Cancel" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "FBuild: Error: BUILD FAILED: Cancel" ) );

    thread.Join();

    // Ensure that processes were killed
    SystemMutex mutex1( "FASTBuildFastCancelTest1" );
    SystemMutex mutex2( "FASTBuildFastCancelTest2" );
    SystemMutex mutex3( "FASTBuildFastCancelTest3" );
    SystemMutex mutex4( "FASTBuildFastCancelTest4" );
    SystemMutex * mutexes[] = { &mutex1, &mutex2, &mutex3, &mutex4 };
    for ( SystemMutex * mutex : mutexes )
    {
        // We should be able to acquire each lock
        // Allow for delays in the mutex being freed as the process was terminated
        uint32_t tryCount = 0;
        while ( mutex->TryLock() == false )
        {
            // Ensure if test is broken that it fails sensibly
            tryCount++;
            TEST_ASSERT( tryCount < 100 );

            // Wait and try again
            Thread::Sleep( 10 );
        }
    }
}

//------------------------------------------------------------------------------
