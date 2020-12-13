// WorkerWindow
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerWindow.h"

// FBuildWorker
#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"
#include "Tools/FBuild/FBuildWorker/Worker/Worker.h"

// OSUI
#include "OSUI/OSDropDown.h"
#include "OSUI/OSFont.h"
#include "OSUI/OSLabel.h"
#include "OSUI/OSListView.h"
#include "OSUI/OSMenu.h"
#include "OSUI/OSSplitter.h"
#include "OSUI/OSTrayIcon.h"
#include "OSUI/OSWindow.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// Defines
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerWindow::WorkerWindow()
    : OSWindow()
    , m_TrayIcon( nullptr )
    , m_Font( nullptr )
    , m_ModeLabel( nullptr )
    , m_ResourcesLabel( nullptr )
    , m_ThreadList( nullptr )
    , m_ModeDropDown( nullptr )
    , m_ResourcesDropDown( nullptr )
    , m_Splitter( nullptr )
    , m_Menu( nullptr )
{
    // center the window on screen
    const uint32_t w = 700;
    const uint32_t h = 300;
    const int32_t x = (int32_t)( GetPrimaryScreenWidth() - w );
    const int32_t y = 0;

    // Create the window
    Init( x, y, w, h );

    // Create the tray icon
    AStackString<> toolTip;
    toolTip.Format( "FBuildWorker %s", FBUILD_VERSION_STRING );
    m_TrayIcon = FNEW( OSTrayIcon( this, toolTip ) );

    // listview
    m_ThreadList = FNEW( OSListView( this ) );
    #if defined( __WINDOWS__ )
        // get main window dimensions for positioning/sizing child controls
        RECT rcClient; // The parent window's client area.
        GetClientRect( (HWND)GetHandle(), &rcClient );
        m_ThreadList->Init( 0, 30, (uint32_t)( rcClient.right - rcClient.left) , (uint32_t)( ( rcClient.bottom - rcClient.top ) - 30 ) );
    #elif defined( __OSX__ )
        m_ThreadList->Init( 4, 30, w - 8, h - 38 );
    #endif
    m_ThreadList->AddColumn( "CPU", 0, 35 );
    m_ThreadList->AddColumn( "Host", 1, 100 );
    m_ThreadList->AddColumn( "Status", 2, 530 );
    size_t numWorkers = JobQueueRemote::Get().GetNumWorkers();
    m_ThreadList->SetItemCount( (uint32_t)numWorkers );
    for ( size_t i = 0; i < numWorkers; ++i )
    {
        AStackString<> string;
        string.Format( "%u", (uint32_t)( i + 1 ) );
        m_ThreadList->AddItem( string.Get() );
    }

    #if defined( __WINDOWS__ )
        // font
        m_Font = FNEW( OSFont() );
        m_Font->Init( 14, "Verdana" );
    #endif

    // Mode drop down
    m_ModeDropDown = FNEW( OSDropDown( this ) );
    m_ModeDropDown->SetFont( m_Font );
    m_ModeDropDown->Init( 100, 3, 200, 200 );
    m_ModeDropDown->AddItem( "Disabled" );
    m_ModeDropDown->AddItem( "Work For Others When Idle" );
    m_ModeDropDown->AddItem( "Work For Others Always" );
    m_ModeDropDown->AddItem( "Work For Others Proportional" );
    m_ModeDropDown->SetSelectedItem( WorkerSettings::Get().GetMode() );

    // Mode label
    m_ModeLabel = FNEW( OSLabel( this ) );
    m_ModeLabel->SetFont( m_Font );
    m_ModeLabel->Init( 5, 7, 95, 15, "Current Mode:" );

    // Threshold drop down
    m_ThresholdDropDown = FNEW( OSDropDown( this ) );
    m_ThresholdDropDown->SetFont( m_Font );
    m_ThresholdDropDown->Init( 376, 3, 67, 200 );
    for ( uint32_t i = 1; i < 6; ++i )
    {
        AStackString<> buffer;
        buffer.Format( "%u%%", i * 10 );
        m_ThresholdDropDown->AddItem( buffer.Get() );
    }
    m_ThresholdDropDown->SetSelectedItem( ( WorkerSettings::Get().GetIdleThresholdPercent() / 10 ) - 1 );
    if ( WorkerSettings::Get().GetMode() != WorkerSettings::WHEN_IDLE )
    {
        m_ThresholdDropDown->SetEnabled( false ); // Only active for Idle mode
    }

    // Threshold label
    m_ThresholdLabel = FNEW( OSLabel( this ) );
    m_ThresholdLabel->SetFont( m_Font );
    m_ThresholdLabel->Init( 305, 7, 66, 15, "Threshold:" );

    // Resources drop down
    m_ResourcesDropDown = FNEW( OSDropDown( this ) );
    m_ResourcesDropDown->SetFont( m_Font );
    m_ResourcesDropDown->Init( 498, 3, 150, 200 );
    {
        // add items
        uint32_t numProcessors = Env::GetNumProcessors();
        AStackString<> buffer;
        for ( uint32_t i = 0; i < numProcessors; ++i )
        {
            float perc = ( i == ( numProcessors - 1 ) ) ? 100.0f : ( (float)( i + 1 ) / (float)numProcessors ) * 100.0f;
            buffer.Format( "%u CPUs (%2.1f%%)", ( i + 1 ), (double)perc );
            m_ResourcesDropDown->AddItem( buffer.Get() );
        }
    }
    m_ResourcesDropDown->SetSelectedItem( WorkerSettings::Get().GetNumCPUsToUse() - 1 );

    // Resources label
    m_ResourcesLabel = FNEW( OSLabel( this ) );
    m_ResourcesLabel->SetFont( m_Font );
    m_ResourcesLabel->Init( 448, 7, 45, 15, "Using:" );

    // splitter
    m_Splitter = FNEW( OSSplitter( this ) );
    m_Splitter->Init( 0, 27, w, 2u );

    // popup menu for tray icon
    m_Menu = FNEW( OSMenu( this ) );
    m_Menu->Init();
    m_Menu->AddItem( "Exit" );
    m_TrayIcon->SetMenu( m_Menu );

    #if defined( __WINDOWS__ )
        // Display the window and minimize it if needed
        if ( WorkerSettings::Get().GetStartMinimzed() )
        {
            UpdateWindow( (HWND)GetHandle() );
            ToggleMinimized(); // minimze
        }
        else
        {
            ShowWindow( (HWND)GetHandle(), SW_SHOWNOACTIVATE );
            UpdateWindow( (HWND)GetHandle() );
            ShowWindow( (HWND)GetHandle(), SW_SHOWNOACTIVATE ); // First call can be ignored
        }
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerWindow::~WorkerWindow()
{
    // clean up UI resources
    FDELETE( m_Splitter );
    FDELETE( m_ResourcesLabel );
    FDELETE( m_ResourcesDropDown );
    FDELETE( m_ThresholdLabel );
    FDELETE( m_ThresholdDropDown );
    FDELETE( m_ModeLabel );
    FDELETE( m_ModeDropDown );
    FDELETE( m_ThreadList );
    FDELETE( m_Menu );
    FDELETE( m_Font );
    FDELETE( m_TrayIcon );
}

// SetStatus
//------------------------------------------------------------------------------
void WorkerWindow::SetStatus( const AString & hostName, const AString & statusText )
{
    AStackString< 512 > text;
    text.Format( "FBuildWorker %s", FBUILD_VERSION_STRING );
    if ( !hostName.IsEmpty() )
    {
        text.AppendFormat( " | \"%s\"", hostName.Get() );
    }
    if ( !statusText.IsEmpty() )
    {
        text.AppendFormat( " | %s", statusText.Get() );
    }
    SetTitle( text.Get() );
}

// SetWorkerState
//------------------------------------------------------------------------------
void WorkerWindow::SetWorkerState( size_t index, const AString & hostName, const AString & status )
{
    m_ThreadList->SetItemText( (uint32_t)index, 1, hostName.Get() );
    m_ThreadList->SetItemText( (uint32_t)index, 2, status.Get() );
}

// Work
//------------------------------------------------------------------------------
void WorkerWindow::Work()
{
    // This will block until the Window closes
    OSWindow::StartMessagePump();
}

// OnMinimize
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerWindow::OnMinimize()
{
    #if defined( __OSX__ )
        SetMinimized( true );
    #else
        // Override minimize
        ToggleMinimized();
    #endif
    return true; // Stop window minimizing (since we already handled it)
}

// OnClose
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerWindow::OnClose()
{
    // Override close to minimize
    #if defined( __OSX__ )
        SetMinimized( true );
    #else
        ToggleMinimized();
    #endif

    return true; // Stop window closeing (since we already handled it)
}

// OnQuit
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerWindow::OnQuit()
{
    Worker::Get().SetWantToQuit();
    return true; // Handled
}

// OnTrayIconLeftClick
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerWindow::OnTrayIconLeftClick()
{
    ToggleMinimized();
    return true; // Handled
}

// OnTrayIconRightClick
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerWindow::OnTrayIconRightClick()
{
    #if defined( __WINDOWS__ )
        uint32_t index;
        if ( m_Menu->ShowAndWaitForSelection( index ) )
        {
            OnTrayIconMenuItemSelected( index );
        }
    #endif

    return true; // Handled
}

// OnDropDownSelectionChanged
//------------------------------------------------------------------------------
/*virtual*/ void WorkerWindow::OnDropDownSelectionChanged( OSDropDown * dropDown )
{
    const size_t index = dropDown->GetSelectedItem();
    if ( dropDown == m_ModeDropDown )
    {
        WorkerSettings::Get().SetMode( (WorkerSettings::Mode)index );

        // Threshold dropdown is enabled when we're in Idle move
        const bool idleMode = ( WorkerSettings::Get().GetMode() == WorkerSettings::WHEN_IDLE );
        m_ThresholdDropDown->SetEnabled( idleMode );
    }
    else if ( dropDown == m_ThresholdDropDown )
    {
        WorkerSettings::Get().SetIdleThresholdPercent( (uint32_t)( index + 1 ) * 10 );
    }
    else if ( dropDown == m_ResourcesDropDown )
    {
        WorkerSettings::Get().SetNumCPUsToUse( (uint32_t)index + 1 );
    }
    WorkerSettings::Get().Save();
}

// OnTrayIconMenuItemSelected
//------------------------------------------------------------------------------
/*virtual*/ void WorkerWindow::OnTrayIconMenuItemSelected( uint32_t /*index*/ )
{
    // We only have one menu item right now
    Worker::Get().SetWantToQuit();
}

// ToggleMinimized
//------------------------------------------------------------------------------
void WorkerWindow::ToggleMinimized()
{
    static bool minimized( false );
    #if defined( __WINDOWS__ )
        if ( !minimized )
        {
            // hide the main window
            ShowWindow( (HWND)GetHandle(), SW_HIDE );
        }
        else
        {
            // show the main window
            HWND hWnd = (HWND)GetHandle();
            ShowWindow( hWnd, SW_SHOW );

            // bring to front
            SetForegroundWindow( hWnd );
            SetActiveWindow( hWnd );
        }
    #elif defined( __APPLE__ )
        SetMinimized( minimized );
    #elif defined( __LINUX__ )
        // TODO:LINUX Implement WorkerWindow::Toggle
    #else
        #error Unknown Platform
    #endif
    minimized = !minimized;

    WorkerSettings::Get().SetStartMinimized( minimized );
    WorkerSettings::Get().Save();
}

//------------------------------------------------------------------------------
