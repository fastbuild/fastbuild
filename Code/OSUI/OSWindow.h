// OSWindow.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class OSDropDown;
class OSWidget;

// Defines
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    // Windows user messages
    #define OSUI_WM_TRAYICON ( WM_USER + 1 )
#endif

// OSWindow
//------------------------------------------------------------------------------
class OSWindow
{
public:
    explicit OSWindow( void * hInstance );
    virtual ~OSWindow();

    void Init( int32_t x, int32_t y, uint32_t w, uint32_t h );

    void AddChild( OSWidget * childWidget );

    #if defined( __WINDOWS__ )
        inline void *   GetHandle() const { return m_Handle; }
        inline void *   GetHInstance() const { return m_HInstance; }

        OSWidget *      GetChildFromHandle( void * handle );
    #endif

    void SetTitle( const char * title );

    // Events for derived classes to respond to
    virtual bool OnMinimize();
    virtual bool OnClose();
    virtual bool OnTrayIconLeftClick();
    virtual bool OnTrayIconRightClick();
    virtual void OnDropDownSelectionChanged( OSDropDown * dropDown );

protected:
    #if defined( __WINDOWS__ )
        void * m_Handle;
        void * m_HInstance;
    #endif
    Array< OSWidget * > m_ChildWidgets;
};

//------------------------------------------------------------------------------
