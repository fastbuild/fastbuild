// TestSplitter.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSSplitter.h"
#include "OSUI/OSWindow.h"

// TestSplitter
//------------------------------------------------------------------------------
class TestSplitter : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void Init() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestSplitter )
    REGISTER_TEST( Empty )
    REGISTER_TEST( Init )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestSplitter::Empty() const
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

// Init
//------------------------------------------------------------------------------
void TestSplitter::Init() const
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
