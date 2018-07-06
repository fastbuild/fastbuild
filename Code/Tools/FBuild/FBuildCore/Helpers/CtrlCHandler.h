// CtrlCHandler
//------------------------------------------------------------------------------
#pragma once

// CtrlCHandler
//------------------------------------------------------------------------------
class CtrlCHandler
{
public:
    CtrlCHandler();
    ~CtrlCHandler();

    void RegisterHandler();
    void DeregisterHandler();
private:
    bool m_IsRegistered = false;
};

//------------------------------------------------------------------------------
