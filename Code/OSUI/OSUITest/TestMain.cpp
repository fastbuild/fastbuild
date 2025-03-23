// TestMain.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// main
//------------------------------------------------------------------------------
int main( int, char *[] )
{
    // Tests to run
    REGISTER_TESTGROUP( TestDropDown )
    REGISTER_TESTGROUP( TestFont )
    REGISTER_TESTGROUP( TestLabel )
    REGISTER_TESTGROUP( TestListView )
    REGISTER_TESTGROUP( TestMenu )
    REGISTER_TESTGROUP( TestSplitter )
    REGISTER_TESTGROUP( TestTrayIcon )
    REGISTER_TESTGROUP( TestWindow )

    TestManager utm;

    const bool allPassed = utm.RunTests();

    return allPassed ? 0 : -1;
}

//------------------------------------------------------------------------------
