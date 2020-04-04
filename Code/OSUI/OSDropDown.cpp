// OSDropDown.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSDropDown.h"

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
    void * DropDownOSX_Create( OSDropDown * owner, int32_t x, int32_t y, uint32_t w, uint32_t h );
    void DropDownOSX_AddItem( OSDropDown * owner, const char * itemText );
    void DropDownOSX_SetSelectedItem( OSDropDown * owner, uint32_t index );
    uint32_t DropDownOSX_GetSelectedItem( const OSDropDown * owner );
    void DropDownOSX_SetEnabled( const OSDropDown * owner, bool enabled );
#endif

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
                                  CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
                                  x, y,
                                  (int32_t)w, (int32_t)h,
                                  (HWND)m_Parent->GetHandle(),
                                  nullptr,
                                  nullptr, // TODO: ??? m_HInstance
                                  nullptr );

        // Font
        SendMessage( (HWND)m_Handle, WM_SETFONT, (WPARAM)m_Font->GetFont(), (LPARAM)0 );
    #elif defined( __OSX__ )
        m_Handle = DropDownOSX_Create( this, x, y, w, h );
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
    #elif defined( __OSX__ )
        DropDownOSX_AddItem( this, itemText );
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
    #elif defined( __OSX__ )
        DropDownOSX_SetSelectedItem( this, index );
    #else
        (void)index;
    #endif
}

// GetSelectedItem
//------------------------------------------------------------------------------
size_t OSDropDown::GetSelectedItem() const
{
    #if defined( __WINDOWS__ )
        return (size_t)SendMessage( (HWND)m_Handle, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0 );
    #elif defined( __OSX__ )
        return DropDownOSX_GetSelectedItem( this );
    #else
        ASSERT( false );
        return 0;
    #endif
}

// SetEnabled
//------------------------------------------------------------------------------
void OSDropDown::SetEnabled( bool enabled )
{
    #if defined( __WINDOWS__ )
        EnableWindow( (HWND)m_Handle, enabled );
    #elif defined( __OSX__ )
        DropDownOSX_SetEnabled( this, enabled );
    #else
        ASSERT( false );
        (void)enabled; // TODO:LINUX EnableWindow equivalent
    #endif
}

//------------------------------------------------------------------------------
