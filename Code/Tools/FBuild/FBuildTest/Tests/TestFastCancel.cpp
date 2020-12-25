// TestFastCancel.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

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
    Timer t;

    // Wait for spawned processes to own all mutexes
    SystemMutex mutex1( "FASTBuildFastCancelTest1" );
    SystemMutex mutex2( "FASTBuildFastCancelTest2" );
    SystemMutex mutex3( "FASTBuildFastCancelTest3" );
    SystemMutex mutex4( "FASTBuildFastCancelTest4" );
    SystemMutex * mutexes[] = { &mutex1, &mutex2, &mutex3, &mutex4 };
    for ( SystemMutex * mutex : mutexes )
    {
        // if we acquired the lock, the child process is not yet spawned
        while ( mutex->TryLock() == true )
        {
            // unlock so child can acquire
            mutex->Unlock();

            // Wait before trying again
            Thread::Sleep( 10 );
            TEST_ASSERT( t.GetElapsed() < 5.0f ); // Ensure test doesn't hang if broken
        }
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

    // We'll coordinate processes with these
    SystemMutex mutex1( "FASTBuildFastCancelTest1" );
    SystemMutex mutex2( "FASTBuildFastCancelTest2" );
    SystemMutex mutex3( "FASTBuildFastCancelTest3" );
    SystemMutex mutex4( "FASTBuildFastCancelTest4" );
    SystemMutex * mutexes[] = { &mutex1, &mutex2, &mutex3, &mutex4 };

    // Create thread that will abort build once all processes are spawned
    Thread::ThreadHandle h = Thread::CreateThread( CancelHelperThread );

    // Start build and check it was aborted
    TEST_ASSERT( fBuild.Build( "Cancel" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "FBuild: Error: BUILD FAILED: Cancel" ) );

    Thread::WaitForThread( h );
    Thread::CloseHandle( h );

    // Ensure that processes were killed
    for ( SystemMutex * mutex : mutexes )
    {
        // We should be able to acquire each lock
        // Allow for delays in the mutex being freed as the process was terminated
        uint32_t tryCount = 0;
        while ( mutex->TryLock() == false )
        {
            // Ensure if test is broken that it fails sensibly
            tryCount++;
            ASSERT( tryCount < 100 );

            // Wait and try again
            Thread::Sleep( 10 );
        }
    }
}

//------------------------------------------------------------------------------
