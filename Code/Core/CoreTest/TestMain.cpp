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
	#if defined( __WINDOWS__ )
		// TODO:MAC Re-enabled TestTestTCPConnectionPool
		// TODO:LINUX Re-enabled TestTestTCPConnectionPool
		REGISTER_TESTGROUP( TestTestTCPConnectionPool )
	#endif
	REGISTER_TESTGROUP( TestTimer )

	UnitTestManager utm;

	bool allPassed = utm.RunTests();

	return allPassed ? 0 : -1;
}

//------------------------------------------------------------------------------
