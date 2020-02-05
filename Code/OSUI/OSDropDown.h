// OSDropDown.h
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

// OSDropDown
//------------------------------------------------------------------------------
class OSDropDown : public OSWidget
{
public:
    explicit OSDropDown( OSWindow * parentWindow );

    void SetFont( OSFont * font );

    void Init( int32_t x, int32_t y, uint32_t w, uint32_t h );

    void AddItem( const char * itemText );
    void SetSelectedItem( size_t index );
    size_t GetSelectedItem() const;

    void SetEnabled( bool enabled );

protected:
    OSFont *    m_Font;
};

//------------------------------------------------------------------------------

