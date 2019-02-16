// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerBrokerage
//------------------------------------------------------------------------------
class WorkerBrokerage
{
public:
    WorkerBrokerage( const AString& cmdlineBrokeragePath );
    ~WorkerBrokerage();

    inline const AString & GetBrokerageRoot() const { return m_BrokerageRoot; }

    // client interface
    void FindWorkers( Array< AString > & workerList, const AString& settingsBrokeragePath );

    // server interface
    void SetAvailability( bool available );
private:
    void Init( const AString& settingsBrokeragePath = AString::GetEmpty() );

    AString             m_CmdlineBrokeragePath;
    AString             m_BrokerageRoot;
    bool                m_Availability;
    bool                m_Initialized;
    AString             m_HostName;
    AString             m_BrokerageFilePath;
    Timer               m_TimerLastUpdate;      // Throttle network access
};

//------------------------------------------------------------------------------
