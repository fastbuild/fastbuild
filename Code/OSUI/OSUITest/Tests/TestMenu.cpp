// TestMenu.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSMenu.h"
#include "OSUI/OSWindow.h"

// TestMenu
//------------------------------------------------------------------------------
class TestMenu : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void Init() const;
    void Items() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestMenu )
    REGISTER_TEST( Empty )
    REGISTER_TEST( Init )
    REGISTER_TEST( Items )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestMenu::Empty() const
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

// Init
//------------------------------------------------------------------------------
void TestMenu::Init() const
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

// Items
//------------------------------------------------------------------------------
void TestMenu::Items() const
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
