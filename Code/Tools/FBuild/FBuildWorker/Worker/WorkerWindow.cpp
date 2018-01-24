// WorkerWindow
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerWindow.h"

#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

// OSUI
#include "OSUI/OSDropDown.h"
#include "OSUI/OSFont.h"
#include "OSUI/OSLabel.h"
#include "OSUI/OSListView.h"
#include "OSUI/OSTrayIcon.h"
#include "OSUI/OSWindow.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/Network/Network.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"

// Windows resources
#if defined( __WINDOWS__ )
    #include "../resource.h"
#endif

// system
#if defined( __WINDOWS__ )
    #include <commctrl.h>
#endif

// Defines
//------------------------------------------------------------------------------
#define ID_TRAY_APP_ICON                5000
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  3000


// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerWindow::WorkerWindow( void * hInstance )
    : OSWindow( hInstance )
    , m_UIState( NOT_READY )
    , m_UIThreadHandle( INVALID_THREAD_HANDLE )
    #if defined( __WINDOWS__ )
        , m_Menu( nullptr )
    #endif
{
    // obtain host name
    Network::GetHostName(m_HostName);

    // spawn UI thread - this creates the window, which must be done
    // on the same thread as we intend to process messages on
    m_UIThreadHandle = Thread::CreateThread( &UIUpdateThreadWrapper,
                                             "UIThread",
                                             ( 32 * KILOBYTE ) );
    ASSERT( m_UIThreadHandle != INVALID_THREAD_HANDLE );

    // wait for the UI thread to hit the main update loop
    while ( m_UIState != UPDATING ) { Thread::Sleep( 1 ); }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerWindow::~WorkerWindow()
{
    // make sure we're only here if we're exiting
    ASSERT( m_UIState == ALLOWED_TO_QUIT );

    // ensure the thread is shutdown
    #if defined( __WINDOWS__ )
        VERIFY( WaitForSingleObject( m_UIThreadHandle, INFINITE ) == WAIT_OBJECT_0 );
    #elif defined( __APPLE__ )
        // TODO:MAC WaitForSingleObject equivalent
    #elif defined( __LINUX__ )
        // TODO:LINUX WaitForSingleObject equivalent
    #else
        #error Unknown Platform
    #endif

    // clean up the ui thread
    Thread::CloseHandle( m_UIThreadHandle );
}

// SetStatus
//------------------------------------------------------------------------------
void WorkerWindow::SetStatus( const char * statusText )
{
    AStackString< 512 > text;
    text.Format( "FBuildWorker %s (%s) | \"%s\" | %s", FBUILD_VERSION_STRING, FBUILD_VERSION_PLATFORM, m_HostName.Get(), statusText );
    SetTitle( text.Get() );
}

// SetWorkerState
//------------------------------------------------------------------------------
void WorkerWindow::SetWorkerState( size_t index, const AString & hostName, const AString & status )
{
    m_ThreadList->SetItemText( (uint32_t)index, 1, hostName.Get() );
    m_ThreadList->SetItemText( (uint32_t)index, 2, status.Get() );
}

// UIUpdateThreadWrapper
//------------------------------------------------------------------------------
/*static*/ uint32_t WorkerWindow::UIUpdateThreadWrapper( void * )
{
    WorkerWindow::Get().UIUpdateThread();
    return 0;
}

// UIUpdateThread
//------------------------------------------------------------------------------
void WorkerWindow::UIUpdateThread()
{
    #if defined( __WINDOWS__ )
        // center the window on screen
        int w = 700;
        int h = 300;
        int x = GetSystemMetrics(SM_CXSCREEN)-w;
        int y = 0;

        Init( x, y, w, h );

        // Create the tray icon
        m_TrayIcon = FNEW( OSTrayIcon( this ) );

        // init windows common controls
        INITCOMMONCONTROLSEX icex; // Structure for control initialization.
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        // get main window dimensions for positioning/sizing child controls
        RECT rcClient; // The parent window's client area.
        GetClientRect( (HWND)GetHandle(), &rcClient );

        // listview
        m_ThreadList = FNEW( OSListView( this ) );
        m_ThreadList->Init( 0, 30, rcClient.right - rcClient.left, ( rcClient.bottom - rcClient.top ) - 30 );
        m_ThreadList->AddColumn( "CPU", 0, 35 );
        m_ThreadList->AddColumn( "Host", 1, 100 );
        m_ThreadList->AddColumn( "Status", 2, 530 );
        size_t numWorkers = JobQueueRemote::Get().GetNumWorkers();
        m_ThreadList->SetItemCount( (uint32_t)numWorkers );
        for ( size_t i=0; i<numWorkers; ++i )
        {
            AStackString<> string;
            string.Format( "%u", (uint32_t)( numWorkers - i ) );
            m_ThreadList->AddItem( string.Get() );
        }

        // font
        m_Font = FNEW( OSFont() );
        m_Font->Init( 14, "Verdana" );

        // Mode drop down
        m_ModeDropDown = FNEW( OSDropDown( this ) );
        m_ModeDropDown->SetFont( m_Font );
        m_ModeDropDown->Init( 100, 3, 200, 200 );
        m_ModeDropDown->AddItem( "Disabled" );
        m_ModeDropDown->AddItem( "Work For Others When Idle" );
        m_ModeDropDown->AddItem( "Work For Others Always" );
        m_ModeDropDown->SetSelectedItem( WorkerSettings::Get().GetMode() );

        // Mode label
        m_ModeLabel = FNEW( OSLabel( this ) );
        m_ModeLabel->SetFont( m_Font );
        m_ModeLabel->Init( 5, 7, 95, 15, "Current Mode:" );

        // Resources drop down
        m_ResourcesDropDown = FNEW( OSDropDown( this ) );
        m_ResourcesDropDown->SetFont( m_Font );
        m_ResourcesDropDown->Init( 350, 3, 150, 200 );
        {
            // add items
            uint32_t numProcessors = Env::GetNumProcessors();
            AStackString<> buffer;
            for ( uint32_t i=0; i<numProcessors; ++i )
            {
                float perc = ( i == ( numProcessors - 1 ) ) ? 100.0f : ( (float)( i + 1 ) / (float)numProcessors ) * 100.0f;
                buffer.Format( "%i CPUs (%2.1f%%)", ( i + 1 ), perc );
                m_ResourcesDropDown->AddItem( buffer.Get() );
            }
        }
        m_ResourcesDropDown->SetSelectedItem( WorkerSettings::Get().GetNumCPUsToUse() - 1 );

        // Resources label
        m_ResourcesLabel = FNEW( OSLabel( this ) );
        m_ResourcesLabel->SetFont( m_Font );
        m_ResourcesLabel->Init( 305, 7, 45, 15, "Using:" );

        // splitter
        {
            int xPos = 0;
            int yPos = 27;
            int width = w;
            int height = 2;
            m_Splitter = CreateWindowEx( WS_EX_TRANSPARENT, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ETCHEDHORZ , xPos, yPos, width, height, (HWND)GetHandle(), NULL, (HINSTANCE)GetHInstance(), NULL );
        }

        // popup menu for tray icon
        m_Menu = CreatePopupMenu();
        AppendMenu( m_Menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, TEXT( "Exit" ) );

        // Display the window or and minimize it.
        if ( WorkerSettings::Get().GetStartMinimzed() )
        {
            UpdateWindow( (HWND)GetHandle() );
            ToggleMinimized(); // minimze
        }
        else
        {
          ShowWindow( (HWND)GetHandle(), SW_SHOW );
          UpdateWindow( (HWND)GetHandle() );
          ShowWindow( (HWND)GetHandle(), SW_SHOW ); // First call can be ignored
        }

        SetStatus( "Idle" );

        // we can now accept manipulation from the main thread
        m_UIState = UPDATING;

        // process messages until wo need to quit
        MSG msg;
        do
        {
            // any messages pending?
            if ( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) )
            {
                // message available, process it
                VERIFY( GetMessage( &msg, NULL, 0, 0 ) != 0 );
                TranslateMessage( &msg );
                DispatchMessage( &msg );
                continue; // immediately handle any new messages
            }
            else
            {
                // no message right now - prevent CPU thrashing by having a sleep
                Sleep( 100 );
            }
        } while ( m_UIState < ALLOWED_TO_QUIT );

        // clean up UI resources
        DestroyWindow( m_Splitter );
        FDELETE( m_ResourcesLabel );
        FDELETE( m_ResourcesDropDown );
        FDELETE( m_ModeLabel );
        FDELETE( m_ModeDropDown );
        FDELETE( m_ThreadList );
        DestroyMenu( m_Menu );
        FDELETE( m_Font );
        FDELETE( m_TrayIcon );

    #elif defined( __APPLE__ )
        // TODO:MAC Implement UIThread
    #elif defined( __LINUX__ )
        // TODO:LINUX Implement UIThread
    #else
        #error Unknown Platform
    #endif
}

// OnMinimize
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerWindow::OnMinimize()
{
    // Override minimize
    ToggleMinimized();
    return true; // Stop window minimizing (since we already handled it)
}

// OnClose
//------------------------------------------------------------------------------
/*virtual*/ bool WorkerWindow::OnClose()
{
    // Override close to minimize
    ToggleMinimized();
    return true; // Stop window closeing (since we already handled it)
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
        // display popup menu at mouse position
        POINT curPoint ;
        GetCursorPos( &curPoint ) ;
        SetForegroundWindow( (HWND)m_Handle );

        // Show menu and block until hidden
        UINT item = TrackPopupMenu( GetMenu(),
                                    TPM_RETURNCMD | TPM_NONOTIFY,
                                    curPoint.x,
                                    curPoint.y,
                                    0,
                                    (HWND)m_Handle,
                                    nullptr );
        if ( item == ID_TRAY_EXIT_CONTEXT_MENU_ITEM )
        {
            SetWantToQuit();
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
    }
    else if ( dropDown == m_ResourcesDropDown )
    {
        WorkerSettings::Get().SetNumCPUsToUse( (uint32_t)index + 1 );
    }
    WorkerSettings::Get().Save();
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
        // TODO:MAC Implement WorkerWindow::Toggle
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
