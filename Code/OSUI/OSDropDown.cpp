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
    #include <stdlib.h>  // for abs()
#endif

// Defines
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSDropDown::OSDropDown( OSWindow * parentWindow )
    : OSWidget( parentWindow )
    , m_Font( nullptr )
    #if defined( __WINDOWS__ )
        , m_LongestItemLength( 0 )
    #endif
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

        // auto-adjust drop down width to fit text
        const size_t itemLength = strlen( itemText );
        if ( itemLength > m_LongestItemLength )
        {
            m_LongestItemLength = itemLength;

            HDC dc = GetDC( (HWND)m_Handle );
            HANDLE oldFont = SelectObject( dc, m_Font->GetFont() );
            RECT rect = { 0, 0, 0, 0 };
            DrawText( dc, itemText, (int)itemLength, &rect, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE );
            SelectObject( dc, oldFont );
            ReleaseDC( (HWND)m_Handle, dc );

            // calculate width, add 4 pixel padding
            const size_t textWidth = abs( rect.right - rect.left ) + 4;
            SendMessage( (HWND)m_Handle, CB_SETDROPPEDWIDTH, textWidth, 0 );
        }
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
