// OSListView.h
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

// OSListView
//------------------------------------------------------------------------------
class OSListView : public OSWidget
{
public:
    explicit OSListView( OSWindow * parentWindow );

    void SetFont( OSFont * font );

    void Init( int32_t x, int32_t y, uint32_t w, uint32_t h );

    void AddColumn( const char * columnHeading, uint32_t columnIndex, uint32_t columnWidth );
    void SetItemCount( uint32_t itemCount );
    void AddItem( const char * itemText );
    void SetItemText( uint32_t index, uint32_t subItemIndex, const char * text );

protected:
    OSFont *    m_Font;
};

//------------------------------------------------------------------------------

