// TestSharedMemory.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include <Core/Env/Assert.h>
#include <Core/Process/SharedMemory.h>
#include <Core/Process/Thread.h>
#include <Core/Strings/AStackString.h>
#include <Core/Time/Timer.h>

#if defined(__LINUX__) || defined(__APPLE__)
	#include <sys/types.h>
	#include <sys/wait.h>
	#include <unistd.h>
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

#if defined(__WINDOWS__)
	// TODO:WINDOWS Test SharedMemory (without fork, so).
#elif defined(__LINUX__) || defined(__APPLE__)
	AStackString<> sharedMemoryName( "FBuild_SHM_Test_" );
	sharedMemoryName += (sizeof(void*) == 8) ? "64_" : "32_";	
	#if defined( DEBUG )
		sharedMemoryName += "Debug";
	#elif defined( RELEASE )
		#if defined( PROFILING_ENABLED )
			sharedMemoryName += "Profile";
		#else
			sharedMemoryName += "Release";
		#endif
	#endif

	int pid = fork();
	
	Timer t;
	t.Start();

	if(pid == 0)
	{
		SharedMemory shm;
		shm.Open(sharedMemoryName.Get(), sizeof(unsigned int));
		unsigned int* magic = static_cast<unsigned int*>(shm.GetPtr());

		// Asserts raise an exception when running unit tests : forked process
		// will not exit cleanly and it will be ASSERTed in the parent process.
		TEST_ASSERT(magic != nullptr);
		
		// Wait for parent to write magic
		while ( *magic != 0xBEEFBEEF )
		{
			Thread::Sleep( 1 );
			TEST_ASSERT( t.GetElapsed() < 1.0f ); // Sanity check timeout
		}
		
		// Write reponse magic
		*magic = 0xB0AFB0AF;
		_exit(0);
	}
	else
	{
		SharedMemory shm;
		shm.Create(sharedMemoryName.Get(), sizeof(unsigned int));
		unsigned int* magic = static_cast<unsigned int*>(shm.GetPtr());
		TEST_ASSERT( magic );
		
		// Signal child
		*magic = 0xBEEFBEEF;
		
		// Wait for response from child
		while ( *magic != 0xB0AFB0AF )
		{
			Thread::Sleep( 1 );
			TEST_ASSERT( t.GetElapsed() < 1.0f ); // Sanity check timeout
		}

		int status;
		TEST_ASSERT(-1 != wait(&status));
		TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0);
		TEST_ASSERT(magic != nullptr);
		TEST_ASSERT(*magic == 0xB0AFB0AF);
	}
#else
	#error Unknown Platform
#endif
}

//------------------------------------------------------------------------------
