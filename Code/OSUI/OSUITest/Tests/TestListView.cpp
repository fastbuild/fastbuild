// TestListView.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSFont.h"
#include "OSUI/OSListView.h"
#include "OSUI/OSWindow.h"

// TestListView
//------------------------------------------------------------------------------
class TestListView : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void Init() const;
    void Items() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestListView )
    REGISTER_TEST( Empty )
    REGISTER_TEST( Init )
    REGISTER_TEST( Items )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestListView::Empty() const
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // base Create/destroy without initialization
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSListView listView( &window );
}

// Init
//------------------------------------------------------------------------------
void TestListView::Init() const
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // Initialize and free
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSListView listView( &window );
    OSFont font;
    font.Init( 14, "Verdana" );
    listView.SetFont( &font );
    listView.Init( 32, 32, 200, 100 );
}

// Items
//------------------------------------------------------------------------------
void TestListView::Items() const
{
#if defined( __OSX__ )
    // OSX main window appears to be leaked by OS
    SetMemoryLeakCheckEnabled( false );
#endif

    // Create dropdown and populate with items

    // Create/Init
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSListView listView( &window );
    OSFont font;
    font.Init( 14, "Verdana" );
    listView.SetFont( &font );
    listView.Init( 32, 32, 200, 100 );

    // Add Columns
    listView.AddColumn( "Column 1", 0, 200 );
    listView.AddColumn( "Column 2", 1, 200 );
    listView.AddColumn( "Column 3", 2, 200 );

    // Add items
    listView.SetItemCount( 2 );
    listView.AddItem( "Item 1" );
    listView.SetItemText( 0, 0, "Item 1 - A" );
    listView.SetItemText( 0, 1, "Item 1 - B" );
    listView.SetItemText( 0, 2, "Item 1 - C" );
    listView.AddItem( "Item 2" );
    listView.SetItemText( 1, 0, "Item 1 - A" );
    listView.SetItemText( 1, 1, "Item 1 - B" );
    listView.SetItemText( 1, 2, "Item 1 - C" );
}

//------------------------------------------------------------------------------
