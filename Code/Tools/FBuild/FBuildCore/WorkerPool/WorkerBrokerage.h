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
    inline const AString & GetBrokerageRootPaths() const { return m_BrokerageRootPaths; }

    // client interface
    void FindWorkers(
        const Array< AString > & excludedWorkers,
        Array< AString > & workers );

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
    void ListDirContents(
        const Array< AString > & paths,
        Array< AString > & contents ) const;
    void GetWorkers(
        const Array< AString > & excludedWorkers,
        Array< AString > & workersToInclude ) const;
    void SetBrokerageRecord();
    void RemoveBrokerageFile();
    void PeriodicCleanup();

    Array<AString>      m_BrokerageRoots;
    AString             m_BrokerageRootPaths;
    bool                m_Availability;
    bool                m_Initialized;
    AString             m_HostName;
    BrokerageRecord     m_BrokerageRecord;
    Timer               m_TimerLastUpdate;      // Throttle network access
    uint64_t            m_SettingsWriteTime;    // FileTime of settings time when last changed
    Timer               m_TimerLastCleanBroker;
};

//------------------------------------------------------------------------------
