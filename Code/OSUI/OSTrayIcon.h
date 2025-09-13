// OSTrayIcon.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#if !defined( __WINDOWS__ )
    #include "Core/Env/Types.h"
#endif

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
#if defined( __WINDOWS__ )
    explicit OSTrayIcon( OSWindow * parentWindow, const AString & toolTip );
#else
    OSTrayIcon( const void * iconImage, size_t iconImageSize );
#endif

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
