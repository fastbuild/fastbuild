// TestMain.cpp : Defines the entry point for the console application.
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// main
//------------------------------------------------------------------------------
int main(int , char * [])
{
	// tests to run
	REGISTER_TESTGROUP( TestBFFParsing )
	REGISTER_TESTGROUP( TestBuildAndLinkLibrary )
	REGISTER_TESTGROUP( TestBuildFBuild )
	REGISTER_TESTGROUP( TestCachePlugin )
	REGISTER_TESTGROUP( TestCLR )
	REGISTER_TESTGROUP( TestCompressor )
	REGISTER_TESTGROUP( TestCopy )
	REGISTER_TESTGROUP( TestCSharp )
	REGISTER_TESTGROUP( TestCUDA )
	REGISTER_TESTGROUP( TestDistributed )
	REGISTER_TESTGROUP( TestDLL )
	REGISTER_TESTGROUP( TestExe )
	REGISTER_TESTGROUP( TestGraph )
	REGISTER_TESTGROUP( TestIncludeParser )
	REGISTER_TESTGROUP( TestPrecompiledHeaders )
	REGISTER_TESTGROUP( TestProjectGeneration )
	REGISTER_TESTGROUP( TestResources )
	REGISTER_TESTGROUP( TestTest )
	REGISTER_TESTGROUP( TestUnity )
	REGISTER_TESTGROUP( TestVariableStack )

	UnitTestManager utm;

	bool allPassed = utm.RunTests();

	return allPassed ? 0 : -1;
}

//------------------------------------------------------------------------------
