// WorkerWindow
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerWindow.h"

#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"

// Windows resources
#if defined( __WINDOWS__ )
    #include "../resource.h"
#endif

// system
#if defined( __WINDOWS__ )
    #include <commctrl.h>
    #include <Winsock2.h>
#endif
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <unistd.h> // for gethostname - TODO: move functionality to Network
#endif

// Defines
//------------------------------------------------------------------------------
#define ID_TRAY_APP_ICON                5000
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  3000
#define WM_TRAYICON ( WM_USER + 1 )

// Toggle
//------------------------------------------------------------------------------
void Toggle()
{
	static bool minimized( false );
	#if defined( __WINDOWS__ )
		if ( !minimized )
		{
			// hide the main window
			ShowWindow( WorkerWindow::Get().GetWindowHandle(), SW_HIDE );

			// balloon
			WorkerWindow::Get().ShowBalloonTip( "FBuildWorker minimized to tray." );
		}
		else
		{
			// show the main window
			HWND hWnd = WorkerWindow::Get().GetWindowHandle();
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

// WorkerWindowWndProc
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
	LRESULT CALLBACK WorkerWindowWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
	    switch( msg )
	    {
			case WM_SYSCOMMAND:
			{
				switch( wParam & 0xfff0 )
				{
					case SC_MINIMIZE:
					{
						Toggle();
						return 0;
					}
					case SC_CLOSE:
					{
						Toggle();
						return 0;
					}
				}
				break;
			}
			case WM_TRAYICON:
			{
				if (lParam == WM_LBUTTONUP)
				{
					Toggle();
					return 0;
				}
				else if (lParam == WM_RBUTTONDOWN)
				{
					// display popup menu at mouse position
					POINT curPoint ;
					GetCursorPos( &curPoint ) ;
					SetForegroundWindow( hwnd );

					// Show menu and block until hidden
					UINT item = TrackPopupMenu( WorkerWindow::Get().GetMenu(),
												TPM_RETURNCMD | TPM_NONOTIFY,
												curPoint.x,
												curPoint.y,
												0,
												hwnd,
												nullptr );
					if ( item == ID_TRAY_EXIT_CONTEXT_MENU_ITEM )
					{
						WorkerWindow::Get().SetWantToQuit();
						return 0;
					}
				}
				break;
			}
		case WM_CLOSE:
			{
				WorkerWindow::Get().SetWantToQuit();
				return 0;
			}
			case WM_COMMAND:
			{
				if( HIWORD(wParam) == CBN_SELCHANGE )
				{
					if ( (HWND)lParam == WorkerWindow::Get().GetModeComboBoxHandle() )
					{
						int index = (int)SendMessage((HWND) lParam, (UINT) CB_GETCURSEL, (WPARAM)0, (LPARAM)0 );
						WorkerSettings::Get().SetMode( (WorkerSettings::Mode)index );
					}
					else
					{
						int index = (int)SendMessage((HWND) lParam, (UINT) CB_GETCURSEL, (WPARAM)0, (LPARAM)0 );
						WorkerSettings::Get().SetNumCPUsToUse( index + 1 );
					}
					WorkerSettings::Get().Save();
					return 0;
				}
			}
			default:
			{
				// nothing...  fall through
			}
	    }

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerWindow::WorkerWindow( void * hInstance )
	: m_UIState( NOT_READY )
	, m_UIThreadHandle( INVALID_THREAD_HANDLE )
	#if defined( __WINDOWS__ )
		, m_WindowHandle( nullptr )
		, m_Menu( nullptr )
		, m_HInstance( (HINSTANCE)hInstance )
	#endif
{
	// obtain host name
	m_HostName = "Unknown Host";
	char buffer[ 512 ];
	if ( ::gethostname( buffer, 512 ) == 0 )
	{
		m_HostName = buffer;
	}

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

	#if defined( __WINDOWS__ )
		VERIFY( SetWindowText( m_WindowHandle, text.Get() ) );
	#elif defined( __APPLE__ )
		// TODO:MAC SetWindowText equivalent
	#elif defined( __LINUX__ )
		// TODO:LINUX SetWindowText equivalent
	#else
		#error Unknown Platform
	#endif
}

// SetWorkerState
//------------------------------------------------------------------------------
void WorkerWindow::SetWorkerState( size_t index, const AString & hostName, const AString & status )
{
	#if defined( __WINDOWS__ )
		(void)index; (void)status;
		LVITEM item;
		memset( &item, 0, sizeof( LVITEM ) );
		item.mask = LVIF_TEXT;
		item.iItem = (uint32_t)index;

		// host name
		item.pszText = (LPSTR)hostName.Get();
		item.iSubItem = 1;
		SendMessage( m_ThreadListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&item );

		// status
		item.pszText = (LPSTR)status.Get();
		item.iSubItem = 2;

		SendMessage( m_ThreadListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&item );
	#elif defined( __APPLE__ )
		// TODO:MAC Implement WorkerWindow::SetWorkerState
	#elif defined( __LINUX__ )
		// TODO:LINUX Implement WorkerWindow::SetWorkerState
	#else
		#error Unknown Platform
	#endif
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
	// Create the Window
	const char * windowClass = "mainWindowClass";

	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = WorkerWindowWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = m_HInstance;
	wc.hIcon         = (HICON)LoadIcon( m_HInstance, MAKEINTRESOURCE(IDI_MAIN_ICON) );
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = windowClass;
	wc.hIconSm       = wc.hIcon;

	if( !RegisterClassEx( &wc ) )
	{
	return;
	}

	// center the window on screen
	int w = 700;
	int h = 300;
	int x = GetSystemMetrics(SM_CXSCREEN)-w;
	int y = 0; // GetSystemMetrics(SM_CYSCREEN)/2-(h/2);

	m_WindowHandle = CreateWindow( windowClass,			// LPCTSTR lpClassName,
								   nullptr,				// LPCTSTR lpWindowName,
								   WS_CAPTION | WS_SYSMENU, // DWORD dwStyle,
								   x,y,					// int x, int y,
								   w, h,				// int nWidth, int nHeight,
								   nullptr,				// HWND hWndParent,
								   nullptr,				// HMENU hMenu,
								   nullptr,				// HINSTANCE hInstance,
								   nullptr );			// LPVOID lpParam
	ASSERT( m_WindowHandle );

	// Create the tray icon
	ZeroMemory( &m_NotifyIconData, sizeof( NOTIFYICONDATA ) );
	m_NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	m_NotifyIconData.hWnd = m_WindowHandle;
	m_NotifyIconData.uID = ID_TRAY_APP_ICON;
	m_NotifyIconData.uFlags = NIF_ICON |	// provide icon
							  NIF_MESSAGE | // want click msgs
							  NIF_TIP;      // provide tool tip
	m_NotifyIconData.uCallbackMessage = WM_TRAYICON; //this message must be handled in hwnd's window procedure. more info below.
	m_NotifyIconData.hIcon = (HICON)LoadIcon( m_HInstance, MAKEINTRESOURCE(IDI_TRAY_ICON) );
	ASSERT( m_NotifyIconData.hIcon );
	AStackString<> toolTip;
	toolTip.Format( "FBuildWorker %s (%s)", FBUILD_VERSION_STRING, FBUILD_VERSION_PLATFORM );
	AString::Copy( toolTip.Get(), m_NotifyIconData.szTip, toolTip.GetLength() + 1 );

	// init windows common controls
	INITCOMMONCONTROLSEX icex;           // Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	// get main window dimensions for positioning/sizing child controls
	RECT rcClient;                       // The parent window's client area.
	GetClientRect( m_WindowHandle, &rcClient ); 

	// listview
	{
		// Create the list-view window in report view with label editing enabled.
		m_ThreadListView = CreateWindow(WC_LISTVIEW, 
										 "zzz",
										 WS_CHILD | LVS_REPORT | WS_VISIBLE | LVS_NOSORTHEADER,
										 0, 30,
										 rcClient.right - rcClient.left,
										 ( rcClient.bottom - rcClient.top ) - 30,
										 m_WindowHandle,
										 nullptr, //(HMENU)IDM_CODE_SAMPLES,
										 nullptr,
										 nullptr); 

		// columns
		const char * cpuText	= "CPU";
		const char * hostText	= "Host";
		const char * statusText	= "Status";
		LV_COLUMN col;
		memset( &col, 0, sizeof( col ) );
		col.mask = LVCF_WIDTH | LVCF_TEXT;
		col.cx = 35;
		col.pszText = const_cast< char * >( cpuText ); // work around lack of const in LV_COLUMN when setting
		SendMessage(m_ThreadListView,LVM_INSERTCOLUMN,0,(LPARAM)&col);
		col.cx = 100;
		col.pszText = const_cast< char * >( hostText ); // work around lack of const in LV_COLUMN when setting
		SendMessage(m_ThreadListView,LVM_INSERTCOLUMN,1,(LPARAM)&col);
		col.cx = 530;
		col.pszText = const_cast< char * >( statusText ); // work around lack of const in LV_COLUMN when setting
		SendMessage(m_ThreadListView,LVM_INSERTCOLUMN,2,(LPARAM)&col);

		size_t numWorkers = JobQueueRemote::Get().GetNumWorkers();
		SendMessage( m_ThreadListView, LVM_SETITEMCOUNT, (WPARAM)numWorkers, (LPARAM)0 );

		for ( size_t i=0; i<numWorkers; ++i )
		{
			LVITEM item;
			memset( &item, 0, sizeof( LVITEM ) );
			item.mask = LVIF_TEXT;
			AStackString<> string;
			string.Format( "%u", (uint32_t)( numWorkers - i ) );
			item.pszText = (LPSTR)string.Get();
			SendMessage( m_ThreadListView, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&item );
		}
	}

	// font
	{
        m_Font = CreateFontA( 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
							  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
							  CLEARTYPE_QUALITY,
							  DEFAULT_PITCH, 
							  "Verdana");
	}

	// Mode drop down
	{
		int xpos = 100;
		int ypos = 3;
		int nwidth = 200;
		int nheight = 200;
		m_ModeComboBox = CreateWindowA( WC_COMBOBOX, TEXT(""), 
										CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | 
										WS_OVERLAPPED | WS_VISIBLE,
			xpos, ypos, nwidth, nheight, m_WindowHandle, NULL, m_HInstance,
			NULL);

		// font
		SendMessage( m_ModeComboBox, WM_SETFONT, (WPARAM)m_Font, NULL );  

		// add items
		SendMessage( m_ModeComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Disabled" );
		SendMessage( m_ModeComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Work For Others When Idle" );
		SendMessage( m_ModeComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"Work For Others Always" );

		// select item
		int index = (int)WorkerSettings::Get().GetMode();
		SendMessage( m_ModeComboBox, CB_SETCURSEL, (WPARAM)index, (LPARAM)0 );
	}

	// Mode label
	{
		int xPos = 5;
		int yPos = 7;
		int width = 95;
		int height = 15;
		m_ModeComboLabel = CreateWindowEx( WS_EX_TRANSPARENT, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_SYSMENU , xPos, yPos, width, height, m_WindowHandle, NULL, m_HInstance, NULL );
		 
		SendMessage( m_ModeComboLabel, WM_SETFONT, (WPARAM)m_Font, NULL );  
		SendMessage( m_ModeComboLabel, WM_SETTEXT, NULL, (LPARAM)"Current Mode:" );
	}

	// Resources drop down
	{
		int xpos = 350;
		int ypos = 3;
		int nwidth = 150;
		int nheight = 200;
		m_ResourcesComboBox = CreateWindowA( WC_COMBOBOX, TEXT(""), 
										CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | 
										WS_OVERLAPPED | WS_VISIBLE,
										xpos, ypos, 
										nwidth, nheight, 
										m_WindowHandle, NULL, m_HInstance,
										NULL);

		// font
		SendMessage( m_ResourcesComboBox, WM_SETFONT, (WPARAM)m_Font, NULL );  

		// add items
		uint32_t numProcessors = Env::GetNumProcessors();
		AStackString<> buffer;
		for ( uint32_t i=0; i<numProcessors; ++i )
		{
			float perc = ( i == ( numProcessors - 1 ) ) ? 100.0f : ( (float)( i + 1 ) / (float)numProcessors ) * 100.0f;
			buffer.Format( "%i CPUs (%2.1f%%)", ( i + 1 ), perc );
			SendMessage( m_ResourcesComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)buffer.Get() );
		}

		// select item
		int index = (int)WorkerSettings::Get().GetNumCPUsToUse();
		SendMessage( m_ResourcesComboBox, CB_SETCURSEL, (WPARAM)( index - 1 ), (LPARAM)0 );
	}

	// Resources label
	{
		int xPos = 305;
		int yPos = 7;
		int width = 45;
		int height = 15;
		m_ResourcesComboLabel = CreateWindowEx( WS_EX_TRANSPARENT, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_SYSMENU , xPos, yPos, width, height, m_WindowHandle, NULL, m_HInstance, NULL );
		 
		SendMessage( m_ResourcesComboLabel, WM_SETFONT, (WPARAM)m_Font, NULL );  
		SendMessage( m_ResourcesComboLabel, WM_SETTEXT, NULL, (LPARAM)"Using:" );
	}

	// splitter
	{
		int xPos = 0;
		int yPos = 27;
		int width = w; 
		int height = 2;
		m_Splitter = CreateWindowEx( WS_EX_TRANSPARENT, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ETCHEDHORZ , xPos, yPos, width, height, m_WindowHandle, NULL, m_HInstance, NULL );
	}

	// popup menu for tray icon
	m_Menu = CreatePopupMenu();
    AppendMenu( m_Menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, TEXT( "Exit" ) );

	// Display tray icon
	Shell_NotifyIcon(NIM_ADD, &m_NotifyIconData);

	// Display the window, and minimize it
	// - we do this so the user can see the application has run
    ShowWindow( m_WindowHandle, SW_SHOW );
    UpdateWindow( m_WindowHandle );
    ShowWindow( m_WindowHandle, SW_SHOW ); // First call can be ignored
	if ( WorkerSettings::Get().GetStartMinimzed() )
	{
		Toggle(); // minimze
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
	if ( m_WindowHandle )
	{
		DestroyWindow( m_Splitter );
		DestroyWindow( m_ResourcesComboLabel );
		DestroyWindow( m_ResourcesComboBox );
		DestroyWindow( m_ModeComboLabel );
		DestroyWindow( m_ModeComboBox );
		DestroyWindow( m_ThreadListView );
		DestroyWindow( m_WindowHandle );
		DestroyMenu( m_Menu );
		DeleteObject( m_Font );

		Shell_NotifyIcon(NIM_DELETE, &m_NotifyIconData);
	}
    #elif defined( __APPLE__ )
	// TODO:MAC Implement UIThread
    #elif defined( __LINUX__ )
	// TODO:LINUX Implement UIThread
    #else
	#error Unknown Platform
    #endif
}

// ShowBalloonTip
//------------------------------------------------------------------------------
void WorkerWindow::ShowBalloonTip( const char * msg )
{
    #if defined( __WINDOWS__ )
	size_t len = strlen( msg );
	ASSERT( len < 256 );
	if ( len < 256 )
	{
		m_NotifyIconData.uFlags = NIF_INFO;
		m_NotifyIconData.dwInfoFlags = NIIF_INFO;
		strncpy_s( WorkerWindow::Get().m_NotifyIconData.szInfo, 256, msg, len );
		Shell_NotifyIcon( NIM_MODIFY, &WorkerWindow::Get().m_NotifyIconData );
	}
    #elif defined( __APPLE__ )
	// TODO:MAC Implement ShowBalloonTip
    #elif defined( __LINUX__ )
	// TODO:LINUX Implement ShowBalloonTip
    #else
	#error Unknown Platform
    #endif
}

//------------------------------------------------------------------------------
