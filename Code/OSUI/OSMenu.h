// OSMenu.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "OSWidget.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------

// OSMenu
//------------------------------------------------------------------------------
class OSMenu : public OSWidget
{
public:
    explicit OSMenu( OSWindow * parentWindow );
    ~OSMenu();

    void Init();

    struct ItemStatus
    {
        uint32_t Index;
        bool     Enabled = true;
    };

    void AddItem( const char * text, uint32_t index );

    bool ShowAndWaitForSelection( const Array< ItemStatus > & itemStatuses, uint32_t & outIndex );

protected:
    #if defined( __WINDOWS__ )
        void * m_Menu;
    #endif
};

//------------------------------------------------------------------------------

