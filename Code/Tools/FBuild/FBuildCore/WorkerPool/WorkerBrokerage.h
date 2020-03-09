// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Tags.h"
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
       FailureBrokerageTimeStamp = 3
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

    void FindWorkers(
        const Array< Tags > & requiredWorkerTagsList,
        const Array< WorkerInfo > & excludedWorkers,
        Array< Array< WorkerInfo > > & workers );

    // server interface
    bool ShouldUpdateAvailability() const;
    void SetAvailable( const Tags & workerTags );
    void SetUnavailable();
private:
    class BrokerageRecord
    {
        public:
            inline explicit BrokerageRecord() {}
            inline virtual ~BrokerageRecord() = default;

            bool operator == ( const BrokerageRecord & other ) const;
            inline bool operator != ( const BrokerageRecord & other ) const { return !(*this == other ); }

            AString         m_DirPath;
            AString         m_FilePath;
    };

    class TagCache
    {
        public:
            inline explicit TagCache() {}
            inline virtual ~TagCache() = default;

            bool operator == ( const AString & other ) const;
            inline bool operator != ( const AString & other ) const { return !(*this == other ); }

            AString         m_Key;
            Tags             m_Tags;
    };

    class WorkerCache
    {
        public:
            inline explicit WorkerCache() {}
            inline virtual ~WorkerCache() = default;

            bool operator == ( const AString & other ) const;
            inline bool operator != ( const AString & other ) const { return !(*this == other ); }

            AString             m_Key;
            Array< WorkerInfo > m_Workers;
    };

    void Init( const bool isWorker );

    void ListDirContents(
        const Array< AString > & roots,
        const Array< AString > & paths,
        const bool includeDirs,
        Array< WorkerInfo > & contents ) const;
    void AddRootWorkers(
        const Array< WorkerInfo > & excludedWorkers,
        bool & rootWorkersValid,
        Array< WorkerInfo > & rootWorkers,
        Array< WorkerInfo > & rootWorkersToAdd ) const;
    void GetTagCache(
        const WorkerInfo & key,
        Array< TagCache > & tagsCache,
        const TagCache * & foundTagCache ) const;
    void GetWorkersCache(
        const AString & key,
        const Array< AString > & dirNameArray,
        Array< WorkerCache > & workersCache,
        const WorkerCache * & foundWorkerCache ) const;
    void CalcWorkerIntersection(
        const Array< WorkerInfo > & workersToAdd,
        Array< WorkerInfo > & workersForJob ) const;
    void GetWorkersToConsider(
        const bool privatePool,
        const bool firstTag,
        const Array< WorkerInfo > & workersForJob,
        const Array< WorkerInfo > & excludedWorkers,
        bool & rootWorkersValid,
        Array< WorkerInfo > & rootWorkers,
        Array< WorkerInfo > & workersToConsider ) const;
    void HandleFoundKey(
        const bool privatePool,
        const bool firstTag,
        const Tag & requiredWorkerTag,
        const Array< WorkerInfo > & excludedWorkers,
        const TagCache * foundTagCache,
        bool & rootWorkersValid,
        Array< WorkerInfo > & rootWorkers,
        Array< WorkerCache > & workersCache,
        Array< WorkerInfo > & workersForJob ) const;
    void HandleNotFoundKey(
        const bool privatePool,
        const bool firstTag,
        const Tag & requiredWorkerTag,
        const Array< WorkerInfo > & excludedWorkers,
        bool & rootWorkersValid,
        Array< WorkerInfo > & rootWorkers,
        Array< WorkerInfo > & workersForJob ) const;
    void GetBrokerageRecordsFromTags(
        const Tags & workerTags,
        Array<BrokerageRecord> & brokerageRecords ) const;
    void GetBrokerageRecordChanges(
        const Tags & removedTags,
        const Tags & addedTags,
        Array<BrokerageRecord> & removedBrokerageRecords,
        Array<BrokerageRecord> & addedBrokerageRecords ) const;
    bool ApplyBrokerageRecordChanges(
            const Array<BrokerageRecord> & removedBrokerageRecords,
            const Array<BrokerageRecord> & addedBrokerageRecords );
    static void RemoveBrokerageFiles( const Array<BrokerageRecord> & brokerageRecords );
    void PeriodicCleanup();

    Array<AString>            m_BrokerageRoots;
    AString                   m_BrokerageRootPaths;
    bool                      m_Availability;
    Status                    m_Status;
    AString                   m_HostName;
    AString                   m_UserName;
    Tags                      m_WorkerTags;
    Array<BrokerageRecord>    m_BrokerageRecords;
    Timer                     m_TimerLastUpdate;      // Throttle network access
    uint64_t                  m_SettingsWriteTime;    // FileTime of settings time when last changed
    Timer                     m_TimerLastCleanBroker;
    Array< AString >          m_FailingBrokerageFiles;
};

//------------------------------------------------------------------------------
