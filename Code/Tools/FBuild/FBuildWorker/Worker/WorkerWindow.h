// WorkerWindow
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// OSUI
#include "OSUI/OSTrayIcon.h"
#include "OSUI/OSWindow.h"

// Core
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

// WorkerWindow
//------------------------------------------------------------------------------
class WorkerWindow : public OSWindow
{
public:
    WorkerWindow();
    virtual ~WorkerWindow() override;

    // Show/Update UI (blocks)
    void Work();

    void SetStatus( const AString & hostName, const AString & statusText );
    void SetWorkerState( size_t index, const AString & hostName, const AString & status );

    const OSMenu * GetMenu() const { return m_Menu; }

private:
    // OSWindow events
    virtual bool OnClose() override;
    virtual bool OnMinimize() override;
    virtual bool OnQuit() override;
    virtual bool OnTrayIconLeftClick() override;
    virtual bool OnTrayIconRightClick() override;
    virtual void OnDropDownSelectionChanged( OSDropDown * dropDown ) override;
    virtual void OnTrayIconMenuItemSelected( uint32_t index ) override;

    // Internal Helpers
    void ToggleMinimized();

    // Window Elements
    OSTrayIcon *        m_TrayIcon;
    OSFont *            m_Font;
    OSLabel *           m_ModeLabel;
    OSLabel *           m_ThresholdLabel;
    OSLabel *           m_ResourcesLabel;
    OSListView *        m_ThreadList;
    OSDropDown *        m_ModeDropDown;
    OSDropDown *        m_ThresholdDropDown;
    OSDropDown *        m_ResourcesDropDown;
    OSSplitter *        m_Splitter;
    OSMenu *            m_Menu;
};

//------------------------------------------------------------------------------
