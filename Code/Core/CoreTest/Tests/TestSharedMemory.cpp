// TestSharedMemory.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include <Core/Env/Assert.h>
#include <Core/Process/Process.h>
#include <Core/Process/SharedMemory.h>
#include <Core/Process/Thread.h>
#include <Core/Strings/AStackString.h>
#include <Core/Time/Timer.h>
#include <Core/Tracing/Tracing.h>

#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

#if !defined( __has_feature )
    #define __has_feature( ... ) 0
#endif

// TestSharedMemory
//------------------------------------------------------------------------------
class TestSharedMemory : public UnitTest
{
private:
    DECLARE_TESTS

    void CreateAccessDestroy() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestSharedMemory )
    REGISTER_TEST( CreateAccessDestroy )
REGISTER_TESTS_END

// CreateAccessDestroy
//------------------------------------------------------------------------------
void TestSharedMemory::CreateAccessDestroy() const
{

#if defined( __WINDOWS__ )
    // TODO:WINDOWS Test SharedMemory (without fork, so).
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    AStackString<> sharedMemoryName;
    sharedMemoryName.Format( "FBuild_SHM_Test_%u", (uint32_t)Process::GetCurrentId() );

    int pid = fork();
    if ( pid == 0 )
    {
        // We don't want the child to interact with the test framework
        #if defined( ASSERTS_ENABLED )
            AssertHandler::SetAssertCallback( nullptr );
        #endif

        Timer t;
        t.Start();

        SharedMemory shm;

        // Wait for parent to create shared memory and open it
        while ( shm.Open( sharedMemoryName.Get(), sizeof(uint32_t) ) == false )
        {
            if ( t.GetElapsed() >= 10.0f ) // Sanity check timeout
            {
                _exit( 1 ); // Parent catches failure
            }
            Thread::Sleep( 1 );
        }

        // Get shared memory pointer
        volatile uint32_t * magic = static_cast<volatile uint32_t *>( shm.GetPtr() );
        if ( magic == nullptr )
        {
            _exit( 2 ); // Parent catches failure
        }

        // Wait for parent to write magic
        while ( *magic != 0xBEEFBEEF )
        {
            if ( t.GetElapsed() >= 10.0f ) // Sanity check timeout
            {
                _exit( 3 ); // Parent catches failure
            }
            Thread::Sleep( 1 );
        }

        // Write response magic
        *magic = 0xB0AFB0AF;
        _exit( 0 );
    }
    else
    {
        Timer t;
        t.Start();

        // Create shared memory
        SharedMemory shm;
        shm.Create( sharedMemoryName.Get(), sizeof(uint32_t) );
        volatile uint32_t * magic = static_cast<volatile uint32_t *>( shm.GetPtr() );
        TEST_ASSERT( magic );

        // Signal child
        *magic = 0xBEEFBEEF;

        // Wait for response from child
        while ( ( *magic != 0xB0AFB0AF ) && ( t.GetElapsed() < 10.0f ) )
        {
            Thread::Sleep( 1 );
        }

        // Check return result
        int status;
        TEST_ASSERT( -1 != wait( &status ) );
        TEST_ASSERT( WIFEXITED( status ) && "Child process didn't terminate cleanly" );
        const uint32_t exitStatus = (uint32_t)WEXITSTATUS( status );
        OUTPUT( "Child exit status: %u", exitStatus );
        TEST_ASSERT( ( exitStatus == 0 ) && "Non-zero exit status from forked child" );

        // Check expected value from child
        TEST_ASSERT( *magic == 0xB0AFB0AF );
    }
#else
    #error Unknown Platform
#endif
}

//------------------------------------------------------------------------------
