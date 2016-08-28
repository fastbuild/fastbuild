// OSLabel.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "OSWidget.h"
// Core
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class OSFont;

// OSTrayIcon
//------------------------------------------------------------------------------
class OSLabel : public OSWidget
{
public:
    explicit OSLabel( OSWindow * parentWindow );

    void SetFont( OSFont * font );

    void Init( int32_t x, int32_t y, uint32_t w, uint32_t h, const char * labelText );

protected:
    OSFont *    m_Font;
};

//------------------------------------------------------------------------------

