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

    inline const AString & GetBrokerageRoot() const { return m_BrokerageRoot; }

    enum BrokerageStatus
    {
       BrokerageUninitialized = 0,
       BrokerageSuccess       = 1,
       BlacklistedWorker      = 2
    };

    inline uint32_t GetBrokerageStatus() const { return m_BrokerageStatus; }

    void GetStatusMsg(
        const uint32_t brokerageStatus,
        AString & statusMsg ) const;

    // client interface
    void FindWorkers( Array< AString > & workerList );
    inline const Array< AString > & GetBlacklistedWorkers() const { return m_BlacklistedWorkers; }
    void BlacklistWorker(
        const AString & workerName,
        const AString & blacklistReason,
        AString & errorMsg );

    // server interface
    void SetAvailability( bool available );
private:
    void Init();
    void ListDirContents( const AString & path, Array< AString > & contents ) const;
    void RefreshBlacklistedWorkers();
    bool GetBlacklistFile(
        const AString & workerName,
        AString & filePath ) const;

    AString             m_BrokerageRoot;
    AString             m_BlacklistDir;
    bool                m_Availability;
    uint32_t            m_BrokerageStatus;
    AString             m_HostName;
    AString             m_UserName;
    AString             m_BrokerageFilePath;
    Timer               m_TimerLastUpdate;      // Throttle network access
    mutable Array< AString >  m_BlacklistedWorkers;
};

//------------------------------------------------------------------------------
