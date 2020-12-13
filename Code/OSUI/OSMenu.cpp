// OSMenu.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSMenu.h"

// OSUI
#include "OSUI/OSWindow.h"

// System
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Defines
//------------------------------------------------------------------------------
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM 3000

// OSX Functions
//------------------------------------------------------------------------------
#if defined( __OSX__ )
    void * MenuOSX_Create( OSWindow * associatedWindow );
    void MenuOSX_Destroy( OSMenu * owner );
    void MenuOSX_AddItem( OSMenu * owner, const char * text );
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSMenu::OSMenu( OSWindow * parentWindow )
    : OSWidget( parentWindow )
    #if defined( __WINDOWS__ )
        , m_Menu( nullptr )
    #endif
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
OSMenu::~OSMenu()
{
    #if defined( __WINDOWS__ )
        DestroyMenu( (HMENU)m_Menu );
    #elif defined( __OSX__ )
        MenuOSX_Destroy( this );
    #endif
}

// Init
//------------------------------------------------------------------------------
void OSMenu::Init()
{
    #if defined( __WINDOWS__ )
        m_Menu = CreatePopupMenu();
    #elif defined( __OSX__ )
        m_Handle = MenuOSX_Create( m_Parent );
    #endif

    OSWidget::Init();
}

// AddItem
//------------------------------------------------------------------------------
void OSMenu::AddItem( const char * text )
{
    #if defined( __WINDOWS__ )
        AppendMenu( (HMENU)m_Menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, TEXT( text ) );
    #elif defined( __OSX__ )
        MenuOSX_AddItem( this, text );
    #else
        (void)text;
    #endif
}

// ShowAndWaitForSelection
//------------------------------------------------------------------------------
bool OSMenu::ShowAndWaitForSelection( uint32_t & outIndex )
{
    #if defined( __WINDOWS__ )
        // display popup menu at mouse position
        POINT curPoint;
        GetCursorPos( &curPoint );
        SetForegroundWindow( (HWND)m_Parent->GetHandle() );

        // Show menu and block until hidden
        // NOTE: TPM_RETURNCMD makes this BOOL return actually a UINT
        const UINT item = (UINT)TrackPopupMenu( (HMENU)m_Menu,
                                                TPM_RETURNCMD | TPM_NONOTIFY,
                                                curPoint.x,
                                                curPoint.y,
                                                0,
                                                (HWND)m_Parent->GetHandle(),
                                                nullptr );
        if ( item == ID_TRAY_EXIT_CONTEXT_MENU_ITEM )
        {
            outIndex = 0;
            return true;
        }
    #else
        (void)outIndex;
    #endif

    return false; // No selection
}

//------------------------------------------------------------------------------
