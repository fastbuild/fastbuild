// WorkerBrokerageServer - Server-side worker registration
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerBrokerage.h"

// Core
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerBrokerageClient
//------------------------------------------------------------------------------
class WorkerBrokerageServer : public WorkerBrokerage
{
public:
    WorkerBrokerageServer();
    ~WorkerBrokerageServer();

    void SetAvailability( bool available );

    const AString & GetHostName() const { return m_HostName; }
    const AString & GetIPAddress() const { return m_IPAddress; }
    void SetIPAddressOverride( const AString & ipAddress );


protected:
    void UpdateBrokerageFilePath();

    Timer               m_TimerLastUpdate;      // Throttle network access
    Timer               m_TimerLastIPUpdate;    // Throttle dns access
    Timer               m_TimerLastCleanBroker;
    uint64_t            m_SettingsWriteTime = 0; // FileTime of settings time when last changed
    bool                m_Available = false;
    AString             m_BrokerageFilePath;
    bool                m_IPAddressOverridden = false;
    AString             m_IPAddress;
    AString             m_DomainName;
    AString             m_HostName;
};

//------------------------------------------------------------------------------
