// OSWindow.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSWindow.h"

// OSUI
#include "OSUI/OSDropDown.h"

#include "Core/Env/Assert.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Defines
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #define IDI_MAIN_ICON 101
#endif

// OSX Functions
//------------------------------------------------------------------------------
#if defined( __OSX__ )
    void * WindowOSX_Create( OSWindow * owner, int32_t x, int32_t y, uint32_t w, uint32_t h );
    void WindowOSX_Destroy( OSWindow * owner );
    uint32_t WindowOSX_GetPrimaryScreenWidth();
    void WindowOSX_MessageLoop();
    void WindowOSX_SetTitle( OSWindow * owner, const char * title );
    void WindowOSX_SetMinimized( bool minimized );
    void WindowOSX_StopMessageLoop();
#endif

// WindowWndProc
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    LRESULT CALLBACK WindowWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        OSWindow * window = reinterpret_cast< OSWindow * >( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );

        switch( msg )
        {
            case WM_SYSCOMMAND:
            {
                switch( wParam & 0xfff0 )
                {
                    case SC_MINIMIZE:
                    {
                        if ( window->OnMinimize() )
                        {
                            return 0; // Handled
                        }
                        break;
                    }
                    case SC_CLOSE:
                    {
                        if ( window->OnClose() )
                        {
                            return 0; // Handled
                        }
                        break;
                    }
                    default:
                    {
                        break; // Ignore
                    }
                }
                break;
            }
            case OSUI_WM_TRAYICON:
            {
                if ( lParam == WM_LBUTTONUP )
                {
                    if ( window->OnTrayIconLeftClick() )
                    {
                        return 0;
                    }
                }
                else if ( lParam == WM_RBUTTONDOWN )
                {
                    if ( window->OnTrayIconRightClick() )
                    {
                        return 0;
                    }
                }
                break;
            }
            case WM_COMMAND:
            {
                if ( HIWORD(wParam) == CBN_SELCHANGE )
                {
                    OSDropDown * dropDown = (OSDropDown *)window->GetChildFromHandle((void *)lParam);
                    window->OnDropDownSelectionChanged( dropDown );
                    return 0;
                }
                break;
            }
            case WM_QUIT:
            {
                if ( window->OnQuit() )
                {
                    return 0; // Handled
                }
                break;
            }
            default:
            {
                // nothing...  fall through
                break;
            }
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSWindow::OSWindow( void * hInstance )
    : m_Handle( nullptr )
    #if defined( __WINDOWS__ )
        , m_HInstance( hInstance )
    #endif
    , m_ChildWidgets( 0, true )
{
    #if defined( __WINDOWS__ )
        // Obtain the executable HINSTANCE if not explictily provided
        if ( m_HInstance == nullptr )
        {
            m_HInstance = GetModuleHandle( nullptr );
            ASSERT( m_HInstance );
        }
    #else
        (void)hInstance;
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
OSWindow::~OSWindow()
{
    #if defined( __WINDOWS__ )
        if ( m_Handle )
        {
            DestroyWindow( (HWND)m_Handle );
        }
    #elif defined( __OSX__ )
        if ( m_Handle )
        {
            WindowOSX_Destroy( this );
        }
    #endif
}

// Init
//------------------------------------------------------------------------------
void OSWindow::Init( int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    #if defined( __WINDOWS__ )
        ASSERT( m_Handle == nullptr );

        // Register Window class
        AStackString<> uniqueWindowClass;
        uniqueWindowClass.Format( "windowClass_%p", (void *)this );

        WNDCLASSEX wc;
        wc.cbSize           = sizeof(WNDCLASSEX);
        wc.style            = 0;
        wc.lpfnWndProc      = WindowWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = sizeof(void *); // For GWLP_USERDATA
        wc.hInstance        = (HINSTANCE)m_HInstance;
        wc.hIcon            = (HICON)LoadIcon( (HINSTANCE)m_HInstance, MAKEINTRESOURCE( IDI_MAIN_ICON ) );
        wc.hCursor          = LoadCursor( nullptr, IDC_ARROW );
        wc.hbrBackground    = (HBRUSH)( COLOR_WINDOW );
        wc.lpszMenuName     = nullptr;
        wc.lpszClassName    = uniqueWindowClass.Get();
        wc.hIconSm          = wc.hIcon;
        VERIFY( RegisterClassEx( &wc ) );

        // Create Window
        m_Handle = CreateWindow( uniqueWindowClass.Get(),   // LPCTSTR lpClassName,
                                 nullptr,                   // LPCTSTR lpWindowName,
                                 WS_CAPTION | WS_SYSMENU,   // DWORD dwStyle,
                                 x, y,                      // int x, int y,
                                 (int32_t)w, (int32_t)h,    // int nWidth, int nHeight,
                                 nullptr,                   // HWND hWndParent,
                                 nullptr,                   // HMENU hMenu,
                                 nullptr,                   // HINSTANCE hInstance,
                                 nullptr );                 // LPVOID lpParam
        ASSERT( m_Handle );

        // Set user data
        SetWindowLongPtr( (HWND)m_Handle, GWLP_USERDATA, (LONG_PTR)this );
        // User data doesn't take effect until you call SetWindowPos
        VERIFY( SetWindowPos( (HWND)m_Handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE ) );
        ASSERT( this == (void *)GetWindowLongPtr( (HWND)m_Handle, GWLP_USERDATA ) );
    #elif defined( __OSX__ )
        m_Handle = WindowOSX_Create( this, x, y, w, h );
    #else
        (void)x;
        (void)y;
        (void)w;
        (void)h;
    #endif
}

// AddChild
//------------------------------------------------------------------------------
void OSWindow::AddChild( OSWidget * childWidget )
{
    ASSERT( m_ChildWidgets.Find( childWidget ) == nullptr );
    m_ChildWidgets.Append( childWidget );
}

// SetTitle
//------------------------------------------------------------------------------
void OSWindow::SetTitle( const char * title )
{
    #if defined( __WINDOWS__ )
        VERIFY( SetWindowText( (HWND)m_Handle, title ) );
    #elif defined( __APPLE__ )
        WindowOSX_SetTitle( this, title );
    #elif defined( __LINUX__ )
        (void)title; // TODO:LINUX SetWindowText equivalent
    #else
        #error Unknown Platform
    #endif
}

// SetMinimized
//------------------------------------------------------------------------------
void OSWindow::SetMinimized( bool minimized )
{
    #if defined( __WINDOWS__ )
        ASSERT( false ); // TODO:B Unify with Windows functionality
        (void)minimized;
    #elif defined( __OSX__ )
        WindowOSX_SetMinimized( minimized );
    #else
        (void)minimized;
    #endif
}

// GetPrimaryScreenWidth
//------------------------------------------------------------------------------
/*static*/ uint32_t OSWindow::GetPrimaryScreenWidth()
{
    #if defined( __WINDOWS__ )
        return (uint32_t)GetSystemMetrics( SM_CXSCREEN );
    #elif defined( __OSX__ )
        return WindowOSX_GetPrimaryScreenWidth();
    #else
        return 1920; // TODO:LINUX Implement
    #endif
}

// StartMessagePump
//------------------------------------------------------------------------------
void OSWindow::StartMessagePump()
{
    #if defined( __WINDOWS__ )
        for ( ;; )
        {
            // Wait for a messsage
            MSG msg;
            const BOOL bRet = GetMessage( &msg, nullptr, 0, 0 );

            // According to MSDN, this "boolean" can contain -1 on error. It seems
            // an error could only occur due to a program bug (like passing an invalid
            // handle) and not during any normal operation.
            // See: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
            ASSERT( bRet != -1 ); (void)bRet;

            // Check for our custom "Stop" message
            if ( msg.message == OSUI_WM_STOPMSGPUMP )
            {
                break;
            }

            // Allow our message pump to do the work, whether it's WM_QUIT (0 bRet) or not (non-zero bRet)
            TranslateMessage( &msg );
            DispatchMessage( &msg ); 
        }
    #elif defined( __OSX__ )
        // This call blocks until messaged by StopMessagePump
        WindowOSX_MessageLoop();
    #endif
}

// StopMessagePump
//------------------------------------------------------------------------------
void OSWindow::StopMessagePump()
{
    // Signal to StartMessagePump that is should exit
    #if defined( __WINDOWS__ )
        PostMessage( (HWND)m_Handle, OSUI_WM_STOPMSGPUMP, 0, 0 );
    #elif defined( __OSX__ )
        WindowOSX_StopMessageLoop();
    #endif
}

// OnMinimize
//------------------------------------------------------------------------------
/*virtual*/ bool OSWindow::OnMinimize()
{
    return true; // Not handled by child class
}

// OnClose
//------------------------------------------------------------------------------
/*virtual*/ bool OSWindow::OnClose()
{
    return false; // Not handled by child class
}

// OnQuit
//------------------------------------------------------------------------------
/*virtual*/ bool OSWindow::OnQuit()
{
    return false; // Not handled by child class
}

// OnTrayIconLeftClick
//------------------------------------------------------------------------------
/*virtual*/ bool OSWindow::OnTrayIconLeftClick()
{
    return false; // Not handled by child class
}

// OnTrayIconRightClick
//------------------------------------------------------------------------------
/*virtual*/ bool OSWindow::OnTrayIconRightClick()
{
    return false; // Not handled by child class
}

// OnDropDownSelectionChanged
//------------------------------------------------------------------------------
/*virtual*/ void OSWindow::OnDropDownSelectionChanged( OSDropDown * dropDown )
{
    (void)dropDown; // Derived class can ignore these events if desired
}

// OnTrayIconMenuItemSelected
//------------------------------------------------------------------------------
/*virtual*/ void OSWindow::OnTrayIconMenuItemSelected( uint32_t /*index*/ )
{
    // Derived class can ignore these events if desired
}

// GetChildFromHandle
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    OSWidget * OSWindow::GetChildFromHandle( void * handle )
    {
        for ( OSWidget * child : m_ChildWidgets )
        {
            if ( child->GetHandle() == handle )
            {
                return child;
            }
        }
        return nullptr;
    }
#endif

//------------------------------------------------------------------------------
