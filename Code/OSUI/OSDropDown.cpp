// OSDropDown.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSUI/PrecompiledHeader.h"
#include "OSDropDown.h"

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
OSDropDown::OSDropDown( OSWindow * parentWindow )
    : OSWidget( parentWindow )
    , m_Font( nullptr )
{
}

// SetFont
//------------------------------------------------------------------------------
void OSDropDown::SetFont( OSFont * font )
{
    ASSERT( !IsInitialized() ); // Change font after Init not currently supported
    m_Font = font;
}

// Init
//------------------------------------------------------------------------------
void OSDropDown::Init( int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    #if defined( __WINDOWS__ )
        InitCommonControls();

        m_Handle = CreateWindowA( WC_COMBOBOX,
                                  "ComboBox",
                                  CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
                                  x, y,
                                  w, h,
                                  (HWND)m_Parent->GetHandle(),
                                  nullptr,
                                  nullptr, // TODO: ??? m_HInstance
                                  nullptr );

        // Font
        SendMessage( (HWND)m_Handle, WM_SETFONT, (WPARAM)m_Font->GetFont(), (LPARAM)0 );
    #else
        (void)x;
        (void)y;
        (void)w;
        (void)h;
    #endif

    OSWidget::Init();
}

// AddItem
//------------------------------------------------------------------------------
void OSDropDown::AddItem( const char * itemText )
{
    #if defined( __WINDOWS__ )
        SendMessage( (HWND)m_Handle, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)itemText );
    #else
        (void)itemText;
    #endif
}

// SetSelectedItem
//------------------------------------------------------------------------------
void OSDropDown::SetSelectedItem( size_t index )
{
    #if defined( __WINDOWS__ )
        SendMessage( (HWND)m_Handle, CB_SETCURSEL, (WPARAM)uint32_t( index ), (LPARAM)0 );
    #else
        (void)index;
    #endif
}

// GetSelectedItem
//------------------------------------------------------------------------------
size_t OSDropDown::GetSelectedItem() const
{
    #if defined( __WINDOWS__ )
        return (size_t)SendMessage((HWND)m_Handle, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0 );
    #else
        ASSERT(false);
        return 0;
    #endif
}

//------------------------------------------------------------------------------
