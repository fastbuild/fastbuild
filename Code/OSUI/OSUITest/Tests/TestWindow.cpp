// TestWindow.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSDropDown.h"
#include "OSUI/OSFont.h"
#include "OSUI/OSWindow.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestWindow, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestWindow, Empty )
{
    // base Create/destroy without initialization
    OSWindow window;
}

//------------------------------------------------------------------------------
TEST_CASE( TestWindow, Init )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // Initialize and free
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestWindow, SetTitle )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    window.SetTitle( "Title" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestWindow, SetMinimized )
{
    // TODO:B Windows functionality doesn't match OSX
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );

    OSWindow window;
    window.Init( 32, 32, 500, 200 );

    window.SetMinimized( true );
    window.SetMinimized( false );
#endif
}

//------------------------------------------------------------------------------
