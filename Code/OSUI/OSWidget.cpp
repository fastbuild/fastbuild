// OSWidget.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSWidget.h"

// OSUI
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

// Static Data
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    /*static*/ bool OSWidget::s_CommonControlsInitialized( false );
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
OSWidget::OSWidget( OSWindow * parentWindow )
    : m_Parent( parentWindow )
    , m_Handle( nullptr )
    , m_Initialized( false )
{
    #if defined( __WINDOWS__ )
        static bool commCtrlInit( false );
        if ( !commCtrlInit )
        {
            // Init windows common controls
            INITCOMMONCONTROLSEX icex;
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx( &icex );
            commCtrlInit = true;
        }
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
OSWidget::~OSWidget()
{
    #if defined( __WINDOWS__ )
        if ( m_Handle )
        {
            DestroyWindow( (HWND)m_Handle );
        }
    #endif
}

// Init
//------------------------------------------------------------------------------
void OSWidget::Init()
{
    ASSERT( !m_Initialized );
    if ( m_Parent )
    {
        m_Parent->AddChild( this );
    }
    m_Initialized = true;
}

// InitCommonControls
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    void OSWidget::InitCommonControls()
    {
        if ( !s_CommonControlsInitialized )
        {
            // Init windows common controls
            INITCOMMONCONTROLSEX icex;
            icex.dwICC = ICC_LISTVIEW_CLASSES;
            InitCommonControlsEx( &icex );
            s_CommonControlsInitialized = true;
        }
    }
#endif

//------------------------------------------------------------------------------
