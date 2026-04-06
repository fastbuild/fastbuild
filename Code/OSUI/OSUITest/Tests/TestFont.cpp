// TestFont.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSFont.h"
#include "OSUI/OSWindow.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestFont, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestFont, Empty )
{
    // base Create/destroy without initialization
    OSFont font;
}

//------------------------------------------------------------------------------
TEST_CASE( TestFont, Init )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // Initialize and free
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSFont font;
    font.Init( 14, "Verdana" );
}

//------------------------------------------------------------------------------
