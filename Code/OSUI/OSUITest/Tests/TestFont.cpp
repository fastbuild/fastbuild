// TestFont.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSFont.h"
#include "OSUI/OSWindow.h"

// TestDropDown
//------------------------------------------------------------------------------
class TestFont : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void Init() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestFont )
    REGISTER_TEST( Empty )
    REGISTER_TEST( Init )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestFont::Empty() const
{
    // base Create/destroy without initialization
    OSFont font;
}

// Init
//------------------------------------------------------------------------------
void TestFont::Init() const
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
