// OSSplitter.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSSplitter.h"

// OSUI
#include "OSUI/OSWindow.h"

// System
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Defines
//------------------------------------------------------------------------------

// OSX Functions
//------------------------------------------------------------------------------
#if defined( __OSX__ )
    void * SplitterOSX_Create( OSSplitter * owner, int32_t x, int32_t y, uint32_t w, uint32_t h );
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSSplitter::OSSplitter( OSWindow * parentWindow )
    : OSWidget( parentWindow )
{
}

// Init
//------------------------------------------------------------------------------
void OSSplitter::Init( int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    #if defined( __WINDOWS__ )
        // Create control
        m_Handle = CreateWindowEx( WS_EX_TRANSPARENT,
                                   "STATIC",
                                   "",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ETCHEDHORZ,
                                   x, y,
                                   (int32_t)w, (int32_t)h,
                                   (HWND)m_Parent->GetHandle(),
                                   nullptr,
                                   (HINSTANCE)m_Parent->GetHInstance(),
                                   nullptr );
    #elif defined( __OSX__ )
        m_Handle = SplitterOSX_Create( this, x, y, w, h );
    #else
        (void)x;
        (void)y;
        (void)w;
        (void)h;
    #endif

    OSWidget::Init();
}

//------------------------------------------------------------------------------
