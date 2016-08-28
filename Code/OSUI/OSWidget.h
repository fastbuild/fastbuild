// OSWidget.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------

// Forward Declarations
//------------------------------------------------------------------------------
class OSWindow;

// OSWidget
//------------------------------------------------------------------------------
class OSWidget
{
public:
    OSWidget( OSWindow * parentWindow );
    virtual ~OSWidget();

    void Init();
    inline bool IsInitialized() const { return m_Initialized; }

    #if defined( __WINDOWS__ )
        inline void * GetHandle() const { return m_Handle; }
    #endif
protected:

    OSWindow *  m_Parent;
    #if defined( __WINDOWS__ )
        void * m_Handle;
    #endif
    bool        m_Initialized;

    #if defined( __WINDOWS__ )
        void InitCommonControls(); // Called by Widgets that need commctl
        static bool s_CommonControlsInitialized;
    #endif
};

//------------------------------------------------------------------------------
