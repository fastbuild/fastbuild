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
    #endif
}

// Init
//------------------------------------------------------------------------------
void OSMenu::Init()
{
    #if defined( __WINDOWS__ )
        m_Menu = CreatePopupMenu();
    #endif

    OSWidget::Init();
}

// AddItem
//------------------------------------------------------------------------------
void OSMenu::AddItem( const char * text, uint32_t index )
{
    #if defined( __WINDOWS__ )
        AppendMenu( (HMENU)m_Menu, MF_STRING, index, TEXT( text ) );
    #else
        (void)text;
    #endif
}

// ShowAndWaitForSelection
//------------------------------------------------------------------------------
bool OSMenu::ShowAndWaitForSelection( const Array< ItemStatus > & itemStatuses, uint32_t & outIndex )
{
    #if defined( __WINDOWS__ )
        // display popup menu at mouse position
        POINT curPoint;
        GetCursorPos( &curPoint );
        SetForegroundWindow( (HWND)m_Menu );

        for ( const ItemStatus & itemStatus : itemStatuses )
        {
            EnableMenuItem( (HMENU)m_Menu, itemStatus.Index, itemStatus.Enabled ? MF_ENABLED : MF_GRAYED );
        }

        // Show menu and block until hidden
        // NOTE: TPM_RETURNCMD makes this BOOL return actually a UINT
        UINT item = (UINT)TrackPopupMenu( (HMENU)m_Menu,
                                          TPM_RETURNCMD | TPM_NONOTIFY,
                                          curPoint.x,
                                          curPoint.y,
                                          0,
                                          (HWND)m_Parent->GetHandle(),
                                          nullptr );
        if ( item != 0 )
        {
            outIndex = item;
            return true;
        }
    #else
        (void)outIndex;
    #endif

    return false; // No selection
}

//------------------------------------------------------------------------------
