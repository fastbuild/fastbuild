// FBuildStats - Repository for build statistics.
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Dependencies;
class Node;

// FBuildStats
//------------------------------------------------------------------------------
struct FBuildStats
{
public:
    FBuildStats();

    void OnBuildStop( Node * node );

    // statistics updated from the main thread

    // time estimation/progress
    uint32_t    m_NodeTimeTotalms;
    uint32_t    m_NodeTimeProgressms;

    // total time spent
    float       m_TotalBuildTime;       // Total time taken
    uint32_t    m_TotalLocalCPUTimeMS;  // Total CPU time on local host
    uint32_t    m_TotalRemoteCPUTimeMS; // Total CPU time on remote workers

    // after the build it complete, accumulate all the stats
    void GatherPostBuildStatistics( Node * node );

    void OutputSummary() const;

    // get the total stats
    uint32_t GetNodesProcessed() const  { return m_Totals.m_NumProcessed; }
    uint32_t GetNodesBuilt() const      { return m_Totals.m_NumBuilt; }
    uint32_t GetCacheHits() const       { return m_Totals.m_NumCacheHits; }
    uint32_t GetCacheMisses() const     { return m_Totals.m_NumCacheMisses; }
    uint32_t GetCacheStores() const     { return m_Totals.m_NumCacheStores; }
    uint32_t GetLightCacheCount() const { return m_Totals.m_NumLightCache; }

    // get stats per node type
    struct Stats;
    const Stats & GetStatsFor( Node::Type nodeType ) const { return m_PerTypeStats[ (size_t)nodeType ]; }

    // track these stats for each node type
    struct Stats
    {
        Stats();

        uint32_t m_NumProcessed;
        uint32_t m_NumBuilt;
        uint32_t m_NumCacheHits;
        uint32_t m_NumCacheMisses;
        uint32_t m_NumCacheStores;
        uint32_t m_NumLightCache;

        uint32_t m_ProcessingTimeMS;
        uint32_t m_NumFailed;
        uint32_t m_CachingTimeMS;
    };

    void FormatTime( float timeInSeconds , AString & buffer  ) const;

    const Node * GetRootNode() const { return m_RootNode; }
    const Array< const Node * > & GetNodesByTime() const { return m_NodesByTime; }

    static inline void SetIgnoreCompilerNodeDeps( bool b ) { s_IgnoreCompilerNodeDeps = b; }
private:
    void GatherPostBuildStatisticsRecurse( Node * node );
    void GatherPostBuildStatisticsRecurse( const Dependencies & dependencies );

    Node * m_RootNode;
    Array< const Node * > m_NodesByTime;

    Stats m_PerTypeStats[ Node::NUM_NODE_TYPES ];
    Stats m_Totals;

    static bool s_IgnoreCompilerNodeDeps;
};

//------------------------------------------------------------------------------
