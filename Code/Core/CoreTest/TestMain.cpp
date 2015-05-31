// TestMain.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"
#include "Core/Reflection/BindReflection.h"

// main
//------------------------------------------------------------------------------
int main(int , char * [])
{
	BindReflection_Core();

	// Tests to run
	REGISTER_TESTGROUP( TestAtomic )
	REGISTER_TESTGROUP( TestAString )
	REGISTER_TESTGROUP( TestFileIO )
	REGISTER_TESTGROUP( TestHash )
	REGISTER_TESTGROUP( TestMemPoolBlock )
	REGISTER_TESTGROUP( TestMutex )
	REGISTER_TESTGROUP( TestPathUtils )
	REGISTER_TESTGROUP( TestReflection )
	REGISTER_TESTGROUP( TestSemaphore )
	REGISTER_TESTGROUP( TestTestTCPConnectionPool )
	REGISTER_TESTGROUP( TestTimer )

	UnitTestManager utm;

	bool allPassed = utm.RunTests();

	return allPassed ? 0 : -1;
}

//------------------------------------------------------------------------------
