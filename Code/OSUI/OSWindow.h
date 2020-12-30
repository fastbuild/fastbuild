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
class OSWindow;

// Defines
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
    // Windows user messages
    #define OSUI_WM_TRAYICON ( WM_USER + 1 )
    #define OSUI_WM_STOPMSGPUMP ( WM_USER + 2 )
#endif

// OSWindow
//------------------------------------------------------------------------------
class OSWindow
{
public:
    explicit OSWindow( void * hInstance = nullptr );
    virtual ~OSWindow();

    void Init( int32_t x, int32_t y, uint32_t w, uint32_t h );

    void AddChild( OSWidget * childWidget );

    inline void *   GetHandle() const { return m_Handle; }

    #if defined( __WINDOWS__ )
        inline void *   GetHInstance() const { return m_HInstance; }

        OSWidget *      GetChildFromHandle( void * handle );
    #endif

    void SetTitle( const char * title );
    void SetMinimized( bool minimized );

    static uint32_t GetPrimaryScreenWidth();

    void StartMessagePump();    // Call from main thread. Blocks.
    void StopMessagePump();     // Call from work thread once it will no longer accesses UI. StartMessagePump will then return.

    // Events for derived classes to respond to
    virtual bool OnMinimize();
    virtual bool OnClose();
    virtual bool OnQuit();
    virtual bool OnTrayIconLeftClick();
    virtual bool OnTrayIconRightClick();
    virtual void OnDropDownSelectionChanged( OSDropDown * dropDown );
    virtual void OnTrayIconMenuItemSelected( uint32_t index );

protected:
    void * m_Handle;
    #if defined( __WINDOWS__ )
        void * m_HInstance;
    #endif
    Array< OSWidget * > m_ChildWidgets;
};

//------------------------------------------------------------------------------
