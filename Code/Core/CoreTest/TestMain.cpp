// TestMain.cpp : Defines the entry point for the console application.
//

#include "TestFramework/UnitTest.h"

int main(int , char * [])
{
	// Tests to run
	REGISTER_TESTGROUP( TestAtomic )
	REGISTER_TESTGROUP( TestAString )
	REGISTER_TESTGROUP( TestFileIO )
	REGISTER_TESTGROUP( TestHash )
	REGISTER_TESTGROUP( TestMemPoolBlock )
	REGISTER_TESTGROUP( TestMutex )
	REGISTER_TESTGROUP( TestPathUtils )
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
