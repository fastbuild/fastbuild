// OSFont.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class OSWindow;

// OSTrayIcon
//------------------------------------------------------------------------------
class OSFont
{
public:
    OSFont();
    ~OSFont();

    void Init( uint32_t size, const char * fontFamily );

    #if defined( __WINDOWS__ )
        inline void * GetFont() { return m_Font; }
    #endif

protected:
    #if defined( __WINDOWS__ )
        void * m_Font;
    #endif
};

//------------------------------------------------------------------------------

