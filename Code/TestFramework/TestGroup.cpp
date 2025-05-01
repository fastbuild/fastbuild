// TestGroup.cpp - interface for a group of related tests
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestGroup.h"

// StaticData
//------------------------------------------------------------------------------
/*static*/ bool TestGroup::sMemoryLeakCheckEnabled = false;

// TestNoReturn
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void TestNoReturn()
    {
        #if defined( __clang__ )
            for (;;) {}
        #endif
    }
#endif

//------------------------------------------------------------------------------
