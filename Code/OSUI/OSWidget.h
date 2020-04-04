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
    explicit OSWidget( OSWindow * parentWindow );
    virtual ~OSWidget();

    void Init();
    inline bool IsInitialized() const { return m_Initialized; }

    inline OSWindow * GetParentWindow() const { return m_Parent; }
    inline void * GetHandle() const { return m_Handle; }

protected:
    OSWindow *  m_Parent;
    void *      m_Handle;
    bool        m_Initialized;

    #if defined( __WINDOWS__ )
        void InitCommonControls(); // Called by Widgets that need commctl
        static bool s_CommonControlsInitialized;
    #endif
};

//------------------------------------------------------------------------------
