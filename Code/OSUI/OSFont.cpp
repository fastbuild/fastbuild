// OSFont.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSUI/PrecompiledHeader.h"
#include "OSFont.h"

// OSUI
#include "OSUI/OSWindow.h"

// Core
#include "Core/Env/Assert.h"

// Defines
//------------------------------------------------------------------------------

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSFont::OSFont()
    #if defined( __WINDOWS__ )
        : m_Font( nullptr )
    #endif
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
OSFont::~OSFont()
{
    #if defined( __WINDOWS__ )
        DeleteObject( (HFONT)m_Font );
    #endif
}

// Init
//------------------------------------------------------------------------------
void OSFont::Init( uint32_t size, const char * fontFamily )
{
    #if defined( __WINDOWS__ )
        m_Font = CreateFontA( size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY,
                              DEFAULT_PITCH,
                              fontFamily );
    #else
        (void)size;
        (void)fontFamily;
    #endif
}

//------------------------------------------------------------------------------
