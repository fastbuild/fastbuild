// OSTrayIcon.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #include <Windows.h> // TODO: Remove need for this
    #include <Shellapi.h> // TODO: Remove need for this
#endif

// Forward Declarations
//------------------------------------------------------------------------------
class OSWindow;

// OSTrayIcon
//------------------------------------------------------------------------------
class OSTrayIcon
{
public:
    explicit OSTrayIcon( OSWindow * parentWindow );
    ~OSTrayIcon();

    void ShowNotification( const char * msg );

protected:
    #if defined( __WINDOWS__ )
        NOTIFYICONDATA m_NotifyIconData; // TODO: Remote use of Shellapi.h from header
    #endif
};

//------------------------------------------------------------------------------

