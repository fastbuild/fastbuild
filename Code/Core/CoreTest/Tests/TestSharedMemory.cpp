// TestSharedMemory.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include <Core/Env/Assert.h>
#include <Core/Process/SharedMemory.h>
#include <Core/Process/SystemMutex.h>
#include <Core/Process/Thread.h>


#if defined(__LINUX__) || defined(__APPLE__)
	#include <sys/types.h>
	#include <wait.h>
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
	SystemMutex mutex("fbuild_shared_memory_test");
	bool locked = mutex.TryLock();

	TEST_ASSERT(locked);

	int pid = fork();

	if(pid == 0)
	{
		while(!mutex.TryLock())
		{
			Thread::Sleep(10);
		}
		SharedMemory shm;
		shm.Open("fbuild_test_shared_memory", sizeof(unsigned int));
		unsigned int* magic = static_cast<unsigned int*>(shm.GetPtr());

		// Asserts raise an exception when running unit tests : forked process
		// will not exit cleanly and it will be ASSERTed in the parent process.
		TEST_ASSERT(magic != nullptr);
		TEST_ASSERT(*magic == 0xBEEFBEEF);
		*magic = 0xB0AFB0AF;
		_exit(0);
	}
	else
	{
		SharedMemory shm;
		shm.Create("fbuild_test_shared_memory", sizeof(unsigned int));
		unsigned int* magic = static_cast<unsigned int*>(shm.GetPtr());

		if(magic != nullptr)
		{
			*magic = 0xBEEFBEEF;
		}

		mutex.Unlock();

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
