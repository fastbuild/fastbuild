// OSListView.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSUI/PrecompiledHeader.h"
#include "OSListView.h"

// OSUI
#include "OSUI/OSFont.h"
#include "OSUI/OSWindow.h"

// Core
#include "Core/Env/Assert.h"

// system
#if defined( __WINDOWS__ )
    #include <CommCtrl.h>
    #include <Windows.h>
#endif

// Defines
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSListView::OSListView( OSWindow * parentWindow ) :
    OSWidget( parentWindow ),
    m_Font( nullptr )
{
}

// SetFont
//------------------------------------------------------------------------------
void OSListView::SetFont( OSFont * font )
{
    ASSERT( !IsInitialized() ); // Change font after Init not currently supported
    m_Font = font;
}

// Init
//------------------------------------------------------------------------------
void OSListView::Init( int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    #if defined( __WINDOWS__ )
        InitCommonControls();

        m_Handle = CreateWindow( WC_LISTVIEW,
                                 "ListView",
                                 WS_CHILD | LVS_REPORT | WS_VISIBLE | LVS_NOSORTHEADER,
                                 x, y,
                                 w, h,
                                 (HWND)m_Parent->GetHandle(),
                                 nullptr,
                                 nullptr,
                                 nullptr );
    #else
        (void)x;
        (void)y;
        (void)w;
        (void)h;
    #endif

    OSWidget::Init();
}

// AddColumn
//------------------------------------------------------------------------------
void OSListView::AddColumn( const char * columnHeading, uint32_t columnIndex, uint32_t columnWidth )
{
    #if defined( __WINDOWS__ )
        LV_COLUMN col;
        memset( &col, 0, sizeof( col ) );
        col.mask = LVCF_WIDTH | LVCF_TEXT;
        col.cx = columnWidth;
        col.pszText = const_cast< char * >( columnHeading ); // work around lack of const in LV_COLUMN when setting
        SendMessage( (HWND)m_Handle, LVM_INSERTCOLUMN, columnIndex, (LPARAM)&col );
    #else
        (void)columnHeading;
        (void)columnIndex;
        (void)columnWidth;
    #endif
}

// SetItemCount
//------------------------------------------------------------------------------
void OSListView::SetItemCount( uint32_t itemCount )
{
    #if defined( __WINDOWS__ )
        SendMessage( (HWND)m_Handle, LVM_SETITEMCOUNT, (WPARAM)itemCount, (LPARAM)0 );
    #else
       (void)itemCount;
    #endif
}

// AddItem
//------------------------------------------------------------------------------
void OSListView::AddItem( const char * itemText )
{
    #if defined( __WINDOWS__ )
        LVITEM item;
        memset( &item, 0, sizeof( LVITEM ) );
        item.mask = LVIF_TEXT;
        item.pszText = const_cast< char * >( itemText ); // work around lack of const
        SendMessage( (HWND)m_Handle, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&item );
    #else
        (void)itemText;
    #endif
}

// SetItemText
//------------------------------------------------------------------------------
void OSListView::SetItemText( uint32_t index, uint32_t subItemIndex, const char * text )
{
    #if defined( __WINDOWS__ )
        LVITEM item;
        memset( &item, 0, sizeof( LVITEM ) );
        item.mask = LVIF_TEXT;
        item.iItem = index;

        // host name
        item.pszText = (LPSTR)text;
        item.iSubItem = subItemIndex;
        SendMessage( (HWND)m_Handle, LVM_SETITEM, (WPARAM)0, (LPARAM)&item );
    #else
        (void)index;
        (void)subItemIndex;
        (void)text;
    #endif
}

//------------------------------------------------------------------------------
