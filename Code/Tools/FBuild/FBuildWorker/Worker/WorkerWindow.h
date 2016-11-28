// WorkerWindow
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// OSUI
#include "OSUI/OSTrayIcon.h"
#include "OSUI/OSWindow.h"

// Core
#include "Core/Containers/Singleton.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"

// system
#if defined( __WINDOWS__ )
    #include <windows.h>
    #include <shellapi.h>
#endif

// Forward Declaration
//------------------------------------------------------------------------------
class OSDropDown;
class OSFont;
class OSLabel;
class OSListView;
class OSTrayIcon;
class OSWindow;

// WorkerWindow
//------------------------------------------------------------------------------
class WorkerWindow : public OSWindow, public Singleton< WorkerWindow >
{
public:
    explicit WorkerWindow( void * hInstance );
    ~WorkerWindow();

    void SetStatus( const char * statusText );
    void SetWorkerState( size_t index, const AString & hostName, const AString & status );

    #if defined( __WINDOWS__ )
        HMENU GetMenu() const { return m_Menu; }
    #endif

    bool WantToQuit() const { return ( m_UIState == WANT_TO_QUIT ); }
    void SetWantToQuit() { m_UIState = WANT_TO_QUIT; }
    void SetAllowQuit() { m_UIState = ALLOWED_TO_QUIT; }

    void ShowBalloonTip( const char * msg );

private:
    static uint32_t UIUpdateThreadWrapper( void * );
    void UIUpdateThread();

    // OSWindow events
    virtual bool OnClose() override;
    virtual bool OnMinimize() override;
    virtual bool OnTrayIconLeftClick() override;
    virtual bool OnTrayIconRightClick() override;
    virtual void OnDropDownSelectionChanged( OSDropDown * dropDown ) override;

    // Internal Helpers
    void ToggleMinimized();

    // UI created/updated in another thread to ensure responsiveness at all times
    enum UIThreadState
    {
        NOT_READY,      // UI is not yet created
        UPDATING,       // UI is created and is being updated
        WANT_TO_QUIT,   // UI wants to close down
        ALLOWED_TO_QUIT // main thread has authorized UI shut down
    };
    volatile UIThreadState  m_UIState;
    Thread::ThreadHandle    m_UIThreadHandle;

    // Window Elements
    OSTrayIcon *        m_TrayIcon;
    OSFont *            m_Font;
    OSLabel *           m_ModeLabel;
    OSLabel *           m_ResourcesLabel;
    OSListView *        m_ThreadList;
    OSDropDown *        m_ModeDropDown;
    OSDropDown *        m_ResourcesDropDown;

    // properties of the window
    #if defined( __WINDOWS__ )
        HMENU           m_Menu;             // right-click context menu
    #endif

    // sub items of the window
    #if defined( __WINDOWS__ )
        HWND            m_Splitter;
    #endif

    AString         m_HostName;
};

//------------------------------------------------------------------------------
