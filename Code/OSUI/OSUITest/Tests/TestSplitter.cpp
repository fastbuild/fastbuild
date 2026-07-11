// TestSplitter.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSSplitter.h"
#include "OSUI/OSWindow.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestSplitter, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestSplitter, Empty )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // base Create/destroy without initialization
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSSplitter splitter( &window );
}

//------------------------------------------------------------------------------
TEST_CASE( TestSplitter, Init )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // Initialize and free
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSSplitter splitter( &window );
    splitter.Init( 32, 32, 200, 4 );
}

//------------------------------------------------------------------------------
