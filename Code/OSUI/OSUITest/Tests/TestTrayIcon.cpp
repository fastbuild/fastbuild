// TestTrayIcon.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// TestFramework
#include "TestFramework/TestGroup.h"

// OSUI
#include "OSUI/OSMenu.h"
#include "OSUI/OSTrayIcon.h"
#include "OSUI/OSWindow.h"

// Core
#include "Core/Strings/AStackString.h"

// Static Data
#if !defined( __WINDOWS__ )
namespace
{
    // Test PNG for OSX
    const uint8_t gIconData[] = { 0x00, 0x00, 0x00, 0x0D, // Length of chunk
                                  0x49, 0x48, 0x44, 0x52, // IHDR chunk
                                  0x00, 0x00, 0x00, 0x01, // Width
                                  0x00, 0x00, 0x00, 0x01, // Height
                                  0x01 };
}
#endif

// TestTrayIcon
//------------------------------------------------------------------------------
class TestTrayIcon : public TestGroup
{
private:
    DECLARE_TESTS

    void Init() const;
    void SetMenu() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestTrayIcon )
    REGISTER_TEST( Init )
    REGISTER_TEST( SetMenu )
REGISTER_TESTS_END

// Init
//------------------------------------------------------------------------------
void TestTrayIcon::Init() const
{
    // TODO:B Fix hard coded assumptions about resource name on Windows
    #if defined( __WINDOWS__ )
        if (( true ))
        {
            return;
        }
    #endif

    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );
    #endif

    // Create with image data
    #if defined( __WINDOWS__ )
        OSWindow window;
        window.Init( 32, 32, 500, 200 );
        OSTrayIcon trayIcon( &window, AStackString<>( "Tooltip" ) );
    #else
        OSTrayIcon trayIcon( gIconData, sizeof( gIconData ) );
    #endif
}

// SetMenu
//------------------------------------------------------------------------------
void TestTrayIcon::SetMenu() const
{
    // TODO:B Fix hard coded assumptions about resource name on Windows
    #if defined( __WINDOWS__ )
        if (( true ))
        {
            return;
        }
    #endif

    #if defined( __OSX__ )
        // OSX main window appears to be leaked by OS
        SetMemoryLeakCheckEnabled( false );
    #endif

    // Create
    OSWindow window;
    window.Init( 32, 32, 500, 200 );
    #if defined( __WINDOWS__ )
        OSTrayIcon trayIcon( &window, AStackString<>( "Tooltip" ) );
    #else
        OSTrayIcon trayIcon( gIconData, sizeof( gIconData ) );
    #endif

    // Create menu
    OSMenu menu( &window );
    menu.AddItem( "Item 1" );
    menu.AddItem( "Item 2" );
    menu.AddItem( "Item 3" );

    // Hookup menu
    trayIcon.SetMenu( &menu );
}

//------------------------------------------------------------------------------
