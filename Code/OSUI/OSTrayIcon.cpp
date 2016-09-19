// OSTrayIcon.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSUI/PrecompiledHeader.h"
#include "OSTrayIcon.h"

// OSUI
#include "OSUI/OSWindow.h"

// Core
#include "Core/Env/Assert.h"

// Defines
//------------------------------------------------------------------------------
#define ID_TRAY_APP_ICON                5000
#define IDI_TRAY_ICON                   102

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSTrayIcon::OSTrayIcon( OSWindow * parentWindow )
{
    #if defined( __WINDOWS__ )
        ZeroMemory( &m_NotifyIconData, sizeof( NOTIFYICONDATA ) );
        m_NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
        m_NotifyIconData.hWnd = (HWND)parentWindow->GetHandle();
        m_NotifyIconData.uID = ID_TRAY_APP_ICON;
        m_NotifyIconData.uFlags = NIF_ICON |    // provide icon
                                  NIF_MESSAGE | // want click msgs
                                  NIF_TIP;      // provide tool tip
        m_NotifyIconData.uCallbackMessage = OSUI_WM_TRAYICON; // Message handled in parentWindow's procedure
        m_NotifyIconData.hIcon = (HICON)LoadIcon( (HINSTANCE)parentWindow->GetHInstance(), MAKEINTRESOURCE(IDI_TRAY_ICON) );
        ASSERT( m_NotifyIconData.hIcon );

        // Display
        Shell_NotifyIcon( NIM_ADD, &m_NotifyIconData );
    #else
        (void)parentWindow;
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
OSTrayIcon::~OSTrayIcon()
{
    #if defined( __WINDOWS__ )
        Shell_NotifyIcon( NIM_DELETE, &m_NotifyIconData );
    #endif
}

// ShowNotification
//------------------------------------------------------------------------------
void OSTrayIcon::ShowNotification( const char * msg )
{
    #if defined( __WINDOWS__ )
        size_t len = strlen( msg );
        ASSERT( len < 256 );
        if ( len < 256 )
        {
            m_NotifyIconData.uFlags = NIF_INFO;
            m_NotifyIconData.dwInfoFlags = NIIF_INFO;
            strncpy_s( m_NotifyIconData.szInfo, 256, msg, len );
            Shell_NotifyIcon( NIM_MODIFY, &m_NotifyIconData );
        }
    #elif defined( __APPLE__ )
        (void)msg; // TODO:MAC Implement ShowBalloonTip
    #elif defined( __LINUX__ )
        (void)msg; // TODO:LINUX Implement ShowBalloonTip
    #endif
}

//------------------------------------------------------------------------------
