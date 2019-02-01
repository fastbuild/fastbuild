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

    inline const AString & GetBrokerageRoot() const { return m_BrokerageRoot; }

    // client interface
    void FindWorkers(
        const Array< Tags > & requiredWorkerTagsList,
        const Array< AString > & excludedWorkers,
        Array< Array< AString > > & workers );

    // server interface
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

            AString               m_Key;
            Array< AString> m_Workers;
    };

    void Init();
    void ListDirContents( const AString & path, const bool includeDirs, Array< AString > & contents ) const;
    void AddRootWorkers(
        const Array< AString > & excludedWorkers,
        bool & rootWorkersValid,
        Array< AString > & rootWorkers,
        Array< AString > & rootWorkersToAdd ) const;
    void GetTagCache(
        const AString & key,
        Array< TagCache > & tagsCache,
        const TagCache * & foundTagCache ) const;
    void GetWorkersCache(
        const AString & key,
        const Array< AString > & dirNameArray,
        Array< WorkerCache > & workersCache,
        const WorkerCache * & foundWorkerCache ) const;
    void CalcWorkerIntersection(
        const Array< AString > & workersToAdd,
        Array< AString > & workersForJob ) const;
    void GetWorkersToConsider(
        const bool privatePool,
        const bool firstTag,
        const Array< AString > & workersForJob,
        const Array< AString > & excludedWorkers,
        bool & rootWorkersValid,
        Array< AString > & rootWorkers,
        Array< AString > & workersToConsider ) const;
    void HandleFoundKey(
        const bool privatePool,
        const bool firstTag,
        const Tag & requiredWorkerTag,
        const Array< AString > & excludedWorkers,
        const TagCache * foundTagCache,
        bool & rootWorkersValid,
        Array< AString > & rootWorkers,
        Array< WorkerCache > & workersCache,
        Array< AString > & workersForJob ) const;
    void HandleNotFoundKey(
        const bool privatePool,
        const bool firstTag,
        const Tag & requiredWorkerTag,
        const Array< AString > & excludedWorkers,
        bool & rootWorkersValid,
        Array< AString > & rootWorkers,
        Array< AString > & workersForJob ) const;
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
    
    AString                m_BrokerageRoot;
    AString                m_TagsRoot;
    bool                   m_Availability;
    bool                   m_Initialized;
    AString                m_HostName;
    Tags                   m_WorkerTags;
    Array<BrokerageRecord> m_BrokerageRecords;
    Timer                  m_TimerLastUpdate;      // Throttle network access
};

//------------------------------------------------------------------------------
