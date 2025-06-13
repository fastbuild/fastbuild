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

// TestWindow
//------------------------------------------------------------------------------
class TestWindow : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void Init() const;
    void SetTitle() const;
    void SetMinimized() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestWindow )
    REGISTER_TEST( Empty )
    REGISTER_TEST( Init )
    REGISTER_TEST( SetTitle )
    REGISTER_TEST( SetMinimized )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestWindow::Empty() const
{
    // base Create/destroy without initialization
    OSWindow window;
}

// Init
//------------------------------------------------------------------------------
void TestWindow::Init() const
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
void TestWindow::SetTitle() const
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
void TestWindow::SetMinimized() const
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
