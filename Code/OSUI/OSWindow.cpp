// OSWindow.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSUI/PrecompiledHeader.h"
#include "OSDropDown.h"
#include "OSWindow.h"

#include "Core/Env/Assert.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include <Windows.h>
#endif

// Defines
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    #define IDI_MAIN_ICON                   101
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
                    }
                    case SC_CLOSE:
                    {
                        if ( window->OnClose() )
                        {
                            return 0; // Handled
                        }
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
                if( HIWORD(wParam) == CBN_SELCHANGE )
                {
                    OSDropDown * dropDown = (OSDropDown *)window->GetChildFromHandle((void *)lParam);
                    window->OnDropDownSelectionChanged( dropDown );
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
OSWindow::OSWindow( void * hInstance ) :
    #if defined( __WINDOWS__ )
        m_Handle( nullptr ),
        m_HInstance( hInstance ),
    #endif
    m_ChildWidgets( 0, true )
{
    #if !defined( __WINDOWS__ )
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
        uniqueWindowClass.Format( "windowClass_%p", this );

        WNDCLASSEX wc;
        wc.cbSize           = sizeof(WNDCLASSEX);
        wc.style            = 0;
        wc.lpfnWndProc      = WindowWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = sizeof(void*); // For GWLP_USERDATA
        wc.hInstance        = (HINSTANCE)m_HInstance;
        wc.hIcon            = (HICON)LoadIcon( (HINSTANCE)m_HInstance, MAKEINTRESOURCE(IDI_MAIN_ICON) );
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = uniqueWindowClass.Get();
        wc.hIconSm          = wc.hIcon;
        VERIFY( RegisterClassEx( &wc ) );

        // Create Window
        m_Handle = CreateWindow( uniqueWindowClass.Get(),   // LPCTSTR lpClassName,
                                 nullptr,                   // LPCTSTR lpWindowName,
                                 WS_CAPTION | WS_SYSMENU,   // DWORD dwStyle,
                                 x, y,                      // int x, int y,
                                 w, h,                      // int nWidth, int nHeight,
                                 nullptr,                   // HWND hWndParent,
                                 nullptr,                   // HMENU hMenu,
                                 nullptr,                   // HINSTANCE hInstance,
                                 nullptr );                 // LPVOID lpParam
        ASSERT( m_Handle );

        // Set user data
        SetWindowLongPtr( (HWND)m_Handle, GWLP_USERDATA, (LONG_PTR)this );
        // User data doesn't take effect until you call SetWindowPos
        VERIFY( SetWindowPos( (HWND)m_Handle, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER ) );
        ASSERT( this == (void*)GetWindowLongPtr( (HWND)m_Handle, GWLP_USERDATA ) );
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
        (void)title; // TODO:MAC SetWindowText equivalent
    #elif defined( __LINUX__ )
        (void)title; // TODO:LINUX SetWindowText equivalent
    #else
        #error Unknown Platform
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
