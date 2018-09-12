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
    WorkerBrokerage();
    ~WorkerBrokerage();

    inline const Array<AString> & GetBrokerageRoots() const { return m_BrokerageRoots; }
    inline const AString & GetBrokerageRootsPaths() const { return m_BrokerageRootsPaths; }

    // client interface
    void FindWorkers( Array< AString > & workerList );

    // server interface
    void SetAvailability( bool available );
private:
    void Init();

    Array<AString>      m_BrokerageRoots;
    AString             m_BrokerageRootsPaths;
    bool                m_Availability;
    bool                m_Initialized;
    AString             m_HostName;
    AString             m_BrokerageFilePath;
    Timer               m_TimerLastUpdate;      // Throttle network access
};

//------------------------------------------------------------------------------
