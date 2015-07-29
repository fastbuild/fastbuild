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
    #if defined( __WINDOWS__ )
    	REGISTER_TESTGROUP( TestCachePlugin ) // TODO:LINUX TODO:MAC Enable
        REGISTER_TESTGROUP( TestCLR )
    #endif
	REGISTER_TESTGROUP( TestCompressor )
	REGISTER_TESTGROUP( TestCopy )
    #if defined( __WINDOWS__ )
        REGISTER_TESTGROUP( TestCSharp )
        REGISTER_TESTGROUP( TestCUDA )
    #endif
    #if defined( __WINDOWS__ )
        REGISTER_TESTGROUP( TestDistributed ) // TODO:LINUX TODO:MAC Enable
        REGISTER_TESTGROUP( TestDLL ) // TODO:LINUX TODO:MAC Enable
        REGISTER_TESTGROUP( TestExe ) // TODO:LINUX TODO:MAC Enable
    #endif
	REGISTER_TESTGROUP( TestGraph )
	REGISTER_TESTGROUP( TestIncludeParser )
    #if defined( __WINDOWS__ )
    	REGISTER_TESTGROUP( TestObjectList ) // TODO:LINUX TODO:MAC Enable
        REGISTER_TESTGROUP( TestPrecompiledHeaders ) // TODO:LINUX TODO:MAC Enable
    #endif
	REGISTER_TESTGROUP( TestProjectGeneration )
    #if defined( __WINDOWS__ )
        REGISTER_TESTGROUP( TestResources )
        REGISTER_TESTGROUP( TestTest ) // TODO:LINUX TODO:MAC Enable
        REGISTER_TESTGROUP( TestUnity ) // TODO:LINUX TODO:MAC Enable
    #endif
	REGISTER_TESTGROUP( TestVariableStack )

	UnitTestManager utm;

	bool allPassed = utm.RunTests();

	return allPassed ? 0 : -1;
}

//------------------------------------------------------------------------------
