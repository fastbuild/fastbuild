// OSTrayIcon.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSTrayIcon.h"

// OSUI
#include "OSUI/OSWindow.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Strings/AString.h"

// System
//------------------------------------------------------------------------------
#if defined( __OSX__ )
    #include <mach-o/getsect.h>
    #include <mach-o/ldsyms.h>
#endif

// Defines
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #define ID_TRAY_APP_ICON                5000
    #define IDI_TRAY_ICON                   102
#endif

// OSX Functions
//------------------------------------------------------------------------------
#if defined( __OSX__ )
void * TrayIconOSX_Create( const void * iconData, size_t iconDataSize );
void TrayIconOSX_SetMenu( OSTrayIcon * owner, OSMenu * menu );
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
OSTrayIcon::OSTrayIcon( OSWindow * parentWindow, const AString & toolTip )
{
    ZeroMemory( &m_NotifyIconData, sizeof( NOTIFYICONDATA ) );
    m_NotifyIconData.cbSize = sizeof( NOTIFYICONDATA );
    m_NotifyIconData.hWnd = (HWND)parentWindow->GetHandle();
    m_NotifyIconData.uID = ID_TRAY_APP_ICON;
    m_NotifyIconData.uFlags = NIF_ICON |    // provide icon
                              NIF_MESSAGE | // want click msgs
                              NIF_TIP;      // provide tool tip
    m_NotifyIconData.uCallbackMessage = OSUI_WM_TRAYICON; // Message handled in parentWindow's procedure
    m_NotifyIconData.hIcon = (HICON)LoadIcon( (HINSTANCE)parentWindow->GetHInstance(), MAKEINTRESOURCE( IDI_TRAY_ICON ) );
    ASSERT( m_NotifyIconData.hIcon );

    if ( toolTip.IsEmpty() == false )
    {
        AString::Copy( toolTip.Get(), m_NotifyIconData.szTip, Math::Min<size_t>( toolTip.GetLength(), sizeof( m_NotifyIconData.szTip ) - 1 ) );
    }

    // Display
    Shell_NotifyIcon( NIM_ADD, &m_NotifyIconData );
}
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#if !defined( __WINDOWS__ )
OSTrayIcon::OSTrayIcon( const void * iconImage, size_t iconImageSize )
{
    #if defined( __OSX__ )
    m_Handle = TrayIconOSX_Create( iconImage, iconImageSize );
    #else
    (void)iconImage;
    (void)iconImageSize;
    #endif
}
#endif

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
    const size_t len = strlen( msg );
    AString::Copy( msg, m_NotifyIconData.szTip, Math::Min<size_t>( len, sizeof( m_NotifyIconData.szTip ) - 1 ) );
    m_NotifyIconData.uFlags = NIF_INFO;
    m_NotifyIconData.dwInfoFlags = NIIF_INFO;
    strncpy_s( m_NotifyIconData.szInfo, 256, msg, len );
    Shell_NotifyIcon( NIM_MODIFY, &m_NotifyIconData );
#elif defined( __APPLE__ )
    (void)msg; // TODO:MAC Implement ShowBalloonTip
#elif defined( __LINUX__ )
    (void)msg; // TODO:LINUX Implement ShowBalloonTip
#endif
}

// SetMenu
//------------------------------------------------------------------------------
void OSTrayIcon::SetMenu( OSMenu * menu )
{
#if defined( __WINDOWS__ )
    (void)menu; //TODO:B unify with Windows
#elif defined( __OSX__ )
    TrayIconOSX_SetMenu( this, menu );
#else
    (void)menu; // TODO:LINUX
#endif
}

//------------------------------------------------------------------------------
