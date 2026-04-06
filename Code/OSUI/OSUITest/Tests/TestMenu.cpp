// TestMenu.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSMenu.h"
#include "OSUI/OSWindow.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestMenu, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestMenu, Empty )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // base Create/destroy without initialization
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSMenu menu( &window );
}

//------------------------------------------------------------------------------
TEST_CASE( TestMenu, Init )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // Initialize and free
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSMenu menu( &window );
    menu.Init();
}

//------------------------------------------------------------------------------
TEST_CASE( TestMenu, Items )
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // Create dropdown and populate with items

    // Create/Init
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSMenu menu( &window );
    menu.Init();

    // Add items
    menu.AddItem( "Item 1" );
    menu.AddItem( "Item 2" );
    menu.AddItem( "Item 3" );
}

//------------------------------------------------------------------------------
