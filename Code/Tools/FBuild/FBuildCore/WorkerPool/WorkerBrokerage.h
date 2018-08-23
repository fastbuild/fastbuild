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

    // client interface
    void FindWorkers(
        const Array< AString > & excludedWorkers,
        Array< AString > & workers );

    const AString & GetBrokerageRoot() const { return m_BrokerageRoot; }

        // server interface
    void SetAvailable();
    void SetUnavailable();
private:
    class BrokerageRecord
    {
        public:
            inline explicit BrokerageRecord() {}
            inline virtual ~BrokerageRecord() = default;

            AString         m_DirPath;
            AString         m_FilePath;
    };

    void Init();
    void ListDirContents( const AString & path, Array< AString > & contents ) const;
    void GetRootWorkers(
        const Array< AString > & excludedWorkers,
        Array< AString > & rootWorkersToInclude ) const;
    void SetBrokerageRecord();
    void RemoveBrokerageFile();
    
    AString                m_BrokerageRoot;
    bool                   m_Availability;
    bool                   m_Initialized;
    AString                m_HostName;
    BrokerageRecord        m_BrokerageRecord;
    Timer                  m_TimerLastUpdate;      // Throttle network access
};

//------------------------------------------------------------------------------
