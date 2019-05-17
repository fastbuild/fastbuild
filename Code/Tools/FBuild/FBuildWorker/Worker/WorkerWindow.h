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

// Forward Declaration
//------------------------------------------------------------------------------
class OSDropDown;
class OSFont;
class OSLabel;
class OSListView;
class OSMenu;
class OSSplitter;
class OSTrayIcon;
class OSWindow;
class WorkerBrokerage;

// WorkerWindow
//------------------------------------------------------------------------------
class WorkerWindow : public OSWindow, public Singleton< WorkerWindow >
{
public:
    explicit WorkerWindow( void * hInstance, WorkerBrokerage * workerBrokerage );
    virtual ~WorkerWindow() override;

    void SetStatus( const char * statusText );
    void SetWorkerState( size_t index, const AString & hostName, const AString & status );

    const OSMenu * GetMenu() const { return m_Menu; }

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
    virtual bool OnQuit() override;
    virtual bool OnTrayIconLeftClick() override;
    virtual bool OnTrayIconRightClick() override;
    virtual void OnDropDownSelectionChanged( OSDropDown * dropDown ) override;

    // Internal Helpers
    void ToggleMinimized();

    enum TrayItems : uint32_t
    {
        TRAYITEM_EXIT = 3000,
        TRAYITEM_OPEN_BROKER_DIRECTORY = 3001
    };

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
    OSSplitter *        m_Splitter;
    OSMenu *            m_Menu;

    WorkerBrokerage *   m_WorkerBrokerage;
};

//------------------------------------------------------------------------------
