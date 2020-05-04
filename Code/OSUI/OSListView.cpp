// OSListView.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSListView.h"

// OSUI
#include "OSUI/OSFont.h"
#include "OSUI/OSWindow.h"

// Core
#include "Core/Env/Assert.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h" // Must be before CommCtrl
    #include <CommCtrl.h>
#endif

// Defines
//------------------------------------------------------------------------------

// OSX Functions
//------------------------------------------------------------------------------
#if defined( __OSX__ )
    void * ListViewOSX_Create( OSListView * owner, int32_t x, int32_t y, uint32_t w, uint32_t h );
    void ListViewOSX_AddColumn( OSListView * owner, const char * columnHeading, uint32_t columnWidth );
    void ListViewOSX_AddItem( OSListView * owner, const char * itemText );
    void ListViewOSX_SetItemText( OSListView * owner, uint32_t rowIndex, uint32_t columnIndex, const char * text );
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSListView::OSListView( OSWindow * parentWindow )
    : OSWidget( parentWindow )
    , m_Font( nullptr )
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
                                 (int32_t)w, (int32_t)h,
                                 (HWND)m_Parent->GetHandle(),
                                 nullptr,
                                 nullptr,
                                 nullptr );
    #elif defined( __OSX__ )
        m_Handle = ListViewOSX_Create( this, x, y, w, h );
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
        col.cx = (int32_t)columnWidth;
        col.pszText = const_cast< char * >( columnHeading ); // work around lack of const in LV_COLUMN when setting
        SendMessage( (HWND)m_Handle, LVM_INSERTCOLUMN, columnIndex, (LPARAM)&col );
    #elif defined( __OSX__ )
        ListViewOSX_AddColumn( this, columnHeading, columnWidth );
        (void)columnIndex;
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
        item.iItem = INT_MAX; // Insert at end
        item.pszText = const_cast< char * >( itemText ); // work around lack of const
        SendMessage( (HWND)m_Handle, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&item );
    #elif defined( __OSX__ )
        ListViewOSX_AddItem( this, itemText );
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
        item.iItem = (int32_t)index;

        // host name
        item.pszText = (LPSTR)text;
        item.iSubItem = (int32_t)subItemIndex;
        SendMessage( (HWND)m_Handle, LVM_SETITEM, (WPARAM)0, (LPARAM)&item );
    #elif defined( __OSX__ )
        ListViewOSX_SetItemText( this, index, subItemIndex, text );
    #else
        (void)index;
        (void)subItemIndex;
        (void)text;
    #endif
}

//------------------------------------------------------------------------------
