// OSTrayIcon.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h" // TODO: Remove need for this
    #include <shellapi.h> // TODO: Remove need for this
#endif

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class OSMenu;
class OSWindow;

// OSTrayIcon
//------------------------------------------------------------------------------
class OSTrayIcon
{
public:
    explicit OSTrayIcon( OSWindow * parentWindow, const AString & toolTip );
    ~OSTrayIcon();

    void ShowNotification( const char * msg );

    void SetMenu( OSMenu * menu );

    #if defined( __OSX__ )
        void * GetHandle() const { return m_Handle; }
    #endif
protected:
    #if defined( __WINDOWS__ )
        NOTIFYICONDATA m_NotifyIconData; // TODO: Remote use of Shellapi.h from header
    #endif
    #if defined( __OSX__ )
        void * m_Handle = nullptr;
    #endif
};

//------------------------------------------------------------------------------

