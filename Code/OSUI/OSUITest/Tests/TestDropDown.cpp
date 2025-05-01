// TestDropDown.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSDropDown.h"
#include "OSUI/OSFont.h"
#include "OSUI/OSWindow.h"

// TestDropDown
//------------------------------------------------------------------------------
class TestDropDown : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void Init() const;
    void Items() const;
    void EnableDisable() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestDropDown )
    REGISTER_TEST( Empty )
    REGISTER_TEST( Init )
    REGISTER_TEST( Items )
    REGISTER_TEST( EnableDisable )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestDropDown::Empty() const
{
    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );
    #endif

    // base Create/destroy without initialization
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSDropDown dropDown( &window );
}

// Init
//------------------------------------------------------------------------------
void TestDropDown::Init() const
{
    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );
    #endif

    // Initialize and free
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSDropDown dropDown( &window );
    OSFont font;
    font.Init( 14, "Verdana" );
    dropDown.SetFont( &font );
    dropDown.Init( 32, 32, 200, 100 );
}

// Items
//------------------------------------------------------------------------------
void TestDropDown::Items() const
{
    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );

        if (( true ))
        {
            return; // TODO:B fix faulty test for OSX
        }
    #endif

    // Create dropdown and populate with items

    // Create/Init
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSDropDown dropDown( &window );
    OSFont font;
    font.Init( 14, "Verdana" );
    dropDown.SetFont( &font );
    dropDown.Init( 32, 32, 200, 100 );

    // Add items
    dropDown.AddItem( "Item 1" );
    dropDown.AddItem( "Item 2" );
    dropDown.AddItem( "Item 3" );
    dropDown.AddItem( "Item 4" );

    // No selected item by default
    TEST_ASSERT( dropDown.GetSelectedItem() == size_t( -1 ) );

    // Modify selection
    dropDown.SetSelectedItem( 2 );
    TEST_ASSERT( dropDown.GetSelectedItem() == 2 );
    dropDown.SetSelectedItem( 0 );
    TEST_ASSERT( dropDown.GetSelectedItem() == 0 );

    // Deselect
    dropDown.SetSelectedItem( size_t( -1 ) );
    TEST_ASSERT( dropDown.GetSelectedItem() == size_t( -1 ) );
}

// EnableDisable
//------------------------------------------------------------------------------
void TestDropDown::EnableDisable() const
{
    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );
    #endif

    // Check enabling/disabling

    // Create/Init
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSDropDown dropDown( &window );
    OSFont font;
    font.Init( 14, "Verdana" );
    dropDown.SetFont( &font );
    dropDown.Init( 32, 32, 200, 100 );

    // Disable/Enable
    dropDown.SetEnabled( false );
    dropDown.SetEnabled( true );
}

//------------------------------------------------------------------------------
