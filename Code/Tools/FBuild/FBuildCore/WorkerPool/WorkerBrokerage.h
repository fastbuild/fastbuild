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

    struct WorkerInfo
    {
        bool operator==(const WorkerInfo& rhs)
        {
            // use case insensitive compare, since hostnames are used as dir names
            return basePath.CompareI(rhs.basePath) == 0 &&
                name.CompareI(rhs.name) == 0;
        }

        AString basePath;
        AString name;
    };

    enum FailureValue
    {
       NoFailure                 = 0,
       FailureNoBrokeragePath    = 1,
       FailureNoBrokerageAccess  = 2, 
       FailureBlacklistedWorker  = 3
    };

    enum StatusValue
    {
       BrokerageUninitialized  = 0,
       BrokerageSuccess        = 1,
       BrokerageFailure        = 2
    };

    struct StatusFailure
    {
        StatusFailure() : failureValue(NoFailure) {}

        WorkerInfo workerInfo;
        FailureValue failureValue;
    };

    struct Status
    {
        Array< StatusFailure > failures;
        StatusValue statusValue;
    };

    inline const Status & GetStatus() const { return m_Status; }

    void GetStatusMsg(
        const Status & status,
        AString & statusMsg ) const;

    // client interface
    void FindWorkers( Array< WorkerInfo > & workerList );
    inline const Array< WorkerInfo > & GetBlacklistedWorkers() const { return m_BlacklistedWorkers; }
    void BlacklistWorker(
        const WorkerInfo & workerInfo,
        const AString & blacklistReason,
        AString & errorMsg );

    // server interface
    bool ShouldUpdateAvailability() const;
    void SetAvailability( bool available );
private:
    void Init();
    void ListDirContents(
        const Array< AString > & roots,
        const Array< AString > & paths,
        Array< WorkerInfo > & contents ) const;
    void RefreshBlacklistedWorkers();
    bool GetBlacklistFile(
        const WorkerInfo & workerInfo,
        AString & filePath ) const;

    Array<AString>      m_BrokerageRoots;
    AString             m_BrokerageRootPaths;
    bool                m_Availability;
    Status              m_Status;
    AString             m_HostName;
    AString             m_UserName;
    AString             m_BrokerageFilePath;
    Timer               m_TimerLastUpdate;      // Throttle network access
    mutable Array< WorkerInfo >  m_BlacklistedWorkers;
    uint64_t            m_SettingsWriteTime;    // FileTime of settings time when last changed
};

//------------------------------------------------------------------------------
