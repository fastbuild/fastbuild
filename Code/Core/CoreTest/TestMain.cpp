// TestMain.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// main
//------------------------------------------------------------------------------
int main( int, char *[] )
{
    // Tests to run
    REGISTER_TESTGROUP( TestArray )
    REGISTER_TESTGROUP( TestAtomic )
    REGISTER_TESTGROUP( TestAString )
    REGISTER_TESTGROUP( TestEnv )
    REGISTER_TESTGROUP( TestFileIO )
    REGISTER_TESTGROUP( TestFileStream )
    REGISTER_TESTGROUP( TestHash )
    REGISTER_TESTGROUP( TestLevenshteinDistance )
    REGISTER_TESTGROUP( TestMemPoolBlock )
    REGISTER_TESTGROUP( TestMutex )
    REGISTER_TESTGROUP( TestPathUtils )
    REGISTER_TESTGROUP( TestReflection )
    REGISTER_TESTGROUP( TestSemaphore )
    REGISTER_TESTGROUP( TestSharedMemory )
    REGISTER_TESTGROUP( TestSmallBlockAllocator )
    REGISTER_TESTGROUP( TestSystemMutex )
    REGISTER_TESTGROUP( TestTestTCPConnectionPool )
    REGISTER_TESTGROUP( TestTimer )

    UnitTestManager utm;

    bool allPassed = utm.RunTests();

    return allPassed ? 0 : -1;
}

//------------------------------------------------------------------------------
