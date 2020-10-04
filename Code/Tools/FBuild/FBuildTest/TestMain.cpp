// TestMain.cpp : Defines the entry point for the console application.
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// main
//------------------------------------------------------------------------------
int main( int, char *[] )
{
    // tests to run
    REGISTER_TESTGROUP( TestAlias )
    REGISTER_TESTGROUP( TestArgs )
    REGISTER_TESTGROUP( TestBFFParsing )
    REGISTER_TESTGROUP( TestBuildAndLinkLibrary )
    REGISTER_TESTGROUP( TestBuildFBuild )
    REGISTER_TESTGROUP( TestCache )
    REGISTER_TESTGROUP( TestCachePlugin )
    REGISTER_TESTGROUP( TestCompilationDatabase )
    REGISTER_TESTGROUP( TestCompiler )
    REGISTER_TESTGROUP( TestCompressor )
    REGISTER_TESTGROUP( TestCopy )
    REGISTER_TESTGROUP( TestDistributed )
    REGISTER_TESTGROUP( TestDLL )
    REGISTER_TESTGROUP( TestError )
    REGISTER_TESTGROUP( TestExe )
    REGISTER_TESTGROUP( TestExec )
    REGISTER_TESTGROUP( TestFastCancel )
    REGISTER_TESTGROUP( TestGraph )
    REGISTER_TESTGROUP( TestIf )
    REGISTER_TESTGROUP( TestIncludeParser )
    REGISTER_TESTGROUP( TestLibrary )
    REGISTER_TESTGROUP( TestLinker )
    REGISTER_TESTGROUP( TestListDependencies )
    REGISTER_TESTGROUP( TestNodeReflection )
    REGISTER_TESTGROUP( TestObject )
    REGISTER_TESTGROUP( TestObjectList )
    REGISTER_TESTGROUP( TestPrecompiledHeaders )
    REGISTER_TESTGROUP( TestProjectGeneration )
    REGISTER_TESTGROUP( TestRemoveDir )
    REGISTER_TESTGROUP( TestTest )
    REGISTER_TESTGROUP( TestTextFile )
    REGISTER_TESTGROUP( TestUnity )
    REGISTER_TESTGROUP( TestUserFunctions )
    REGISTER_TESTGROUP( TestVariableStack )
    REGISTER_TESTGROUP( TestWarnings )

    // Windows-specific tests
    #if defined( __WINDOWS__ )
        REGISTER_TESTGROUP( TestCLR )
        REGISTER_TESTGROUP( TestCSharp )
        REGISTER_TESTGROUP( TestCUDA )
        REGISTER_TESTGROUP( TestResources )
        REGISTER_TESTGROUP( TestZW )
    #endif

    UnitTestManager utm;

    bool allPassed = utm.RunTests();

    return allPassed ? 0 : -1;
}

//------------------------------------------------------------------------------
