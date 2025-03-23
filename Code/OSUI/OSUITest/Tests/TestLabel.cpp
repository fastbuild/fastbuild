// TestLabel.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSFont.h"
#include "OSUI/OSLabel.h"
#include "OSUI/OSWindow.h"

// TestDropDown
//------------------------------------------------------------------------------
class TestLabel : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void Init() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestLabel )
    REGISTER_TEST( Empty )
    REGISTER_TEST( Init )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestLabel::Empty() const
{
    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );
    #endif

    // base Create/destroy without initialization
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSLabel label( &window );
}

// Init
//------------------------------------------------------------------------------
void TestLabel::Init() const
{
    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );
    #endif

    // Initialize and free
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    OSLabel label( &window );
    OSFont font;
    font.Init( 14, "Verdana" );
    label.SetFont( &font );
    label.Init( 32, 32, 200, 16, "Text" );
}

//------------------------------------------------------------------------------
