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
    bool IsInitialized() const { return m_Initialized; }

    OSWindow * GetParentWindow() const { return m_Parent; }
    void * GetHandle() const { return m_Handle; }

protected:
    OSWindow * m_Parent = nullptr;
    void * m_Handle = nullptr;
    bool m_Initialized = false;

#if defined( __WINDOWS__ )
    void InitCommonControls(); // Called by Widgets that need commctl
    static bool s_CommonControlsInitialized;
#endif
};

//------------------------------------------------------------------------------
