// OSSplitter.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "OSWidget.h"

// Core
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------

// OSSplitter
//------------------------------------------------------------------------------
class OSSplitter : public OSWidget
{
public:
    explicit OSSplitter( OSWindow * parentWindow );

    void Init( int32_t x, int32_t y, uint32_t w, uint32_t h );
};

//------------------------------------------------------------------------------

