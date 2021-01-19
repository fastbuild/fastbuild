// OSMenu.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "OSWidget.h"

// Core
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------

// OSMenu
//------------------------------------------------------------------------------
class OSMenu : public OSWidget
{
public:
    explicit OSMenu( OSWindow * parentWindow );
    virtual ~OSMenu() override;

    void Init();

    void AddItem( const char * text );

    bool ShowAndWaitForSelection( uint32_t & outIndex );

protected:
    #if defined( __WINDOWS__ )
        void * m_Menu;
    #endif
};

//------------------------------------------------------------------------------

