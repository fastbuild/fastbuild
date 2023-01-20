// Report
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Report.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"

// Core
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args
#include <string.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
Report::Report(size_t initialCapacity, bool resizeable)
    : m_LibraryStats(initialCapacity, resizeable)
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Report::~Report() 
{
    const LibraryStats* const* end = m_LibraryStats.End();
    for (LibraryStats** it = m_LibraryStats.Begin(); it != end; ++it)
    {
        FDELETE* it;
    }
}

// Write
//------------------------------------------------------------------------------
void Report::Write(MSVC_SAL_PRINTF const char* fmtString, ...)
{
    AStackString< 1024 > tmp;

    va_list args;
    va_start( args, fmtString );
    tmp.VFormat( fmtString, args );
    va_end( args );

    // resize output buffer in large chunks to prevent re-sizing
    if ( m_Output.GetLength() + tmp.GetLength() > m_Output.GetReserved() )
    {
        m_Output.SetReserved( m_Output.GetReserved() + MEGABYTE );
    }

    m_Output += tmp;
}

// GetLibraryStats
//------------------------------------------------------------------------------
void Report::GetLibraryStats( const NodeGraph & nodeGraph, const FBuildStats & stats )
{
    // Mark all nodes for recursion sweep
    nodeGraph.SetBuildPassTagForAllNodes( eNodeNotSeen );

    // gather library stats, sorted by CPU cost
    GetLibraryStatsRecurse( m_LibraryStats, stats.GetRootNode(), nullptr );
    m_LibraryStats.SortDeref();
}

// GetLibraryStatsRecurse
//------------------------------------------------------------------------------
void Report::GetLibraryStatsRecurse(Array< LibraryStats* >& libStats, const Node* node, LibraryStats* currentLib) const
{
    // skip nodes we've already seen
    if ( node->GetBuildPassTag() == eNodeSeen )
    {
        return;
    }
    node->SetBuildPassTag( eNodeSeen );

    const Node::Type type = node->GetType();

    // object?
    if ( type == Node::OBJECT_NODE )
    {
        if ( currentLib == nullptr )
        {
            ASSERT( false ); // should not be possible with a correctly formed dep graph
            return;
        }

        currentLib->objectCount++;

        const bool cacheHit = node->GetStatFlag( Node::STATS_CACHE_HIT );
        const bool cacheMiss = node->GetStatFlag( Node::STATS_CACHE_MISS );
        if ( cacheHit || cacheMiss )
        {
            currentLib->objectCount_Cacheable++;

            if ( cacheHit )
            {
                currentLib->objectCount_CacheHits++;
            }
            if ( node->GetStatFlag(Node::STATS_CACHE_STORE) )
            {
                currentLib->objectCount_CacheStores++;
                currentLib->cacheTimeMS += node->GetCachingTime();
            }
        }

        if ( cacheHit || cacheMiss || node->GetStatFlag(Node::STATS_BUILT) )
        {
            currentLib->objectCount_OutOfDate++;
            currentLib->cpuTimeMS += node->GetProcessingTime();
        }

        return; // Stop recursing at Objects
    }

    bool isLibrary = false;
    switch ( type )
    {
    case Node::DLL_NODE:        isLibrary = true; break;
    case Node::LIBRARY_NODE:    isLibrary = true; break;
    case Node::OBJECT_LIST_NODE: isLibrary = true; break;
    case Node::CS_NODE:
    {
        isLibrary = node->GetName().EndsWithI( ".dll" ); // TODO:C - robustify this (could have an aribtrary extension)
        break;
    }
    default: break; // not a library
    }

    if ( isLibrary )
    {
        currentLib = FNEW( LibraryStats );
        currentLib->library = node;
        currentLib->cpuTimeMS = 0;
        currentLib->objectCount = 0;
        currentLib->objectCount_OutOfDate = 0;
        currentLib->objectCount_Cacheable = 0;
        currentLib->objectCount_CacheHits = 0;
        currentLib->objectCount_CacheStores = 0;
        currentLib->cacheTimeMS = 0;

        // count time for library/dll itself
        if ( node->GetStatFlag(Node::STATS_BUILT) || node->GetStatFlag(Node::STATS_FAILED) )
        {
            currentLib->cpuTimeMS += node->GetProcessingTime();
        }

        libStats.Append( currentLib );

        // recurse into this new lib
    }

    // Dependencies
    GetLibraryStatsRecurse( libStats, node->GetPreBuildDependencies(), currentLib );
    GetLibraryStatsRecurse( libStats, node->GetStaticDependencies(), currentLib );
    GetLibraryStatsRecurse( libStats, node->GetDynamicDependencies(), currentLib );
}

// GetLibraryStatsRecurse
//------------------------------------------------------------------------------
void Report::GetLibraryStatsRecurse(Array< LibraryStats* >& libStats, const Dependencies& dependencies, LibraryStats* currentLib) const
{
    const Dependency* const end = dependencies.End();
    for ( const Dependency* it = dependencies.Begin(); it != end; ++it )
    {
        GetLibraryStatsRecurse( libStats, it->GetNode(), currentLib );
    }
}


// GetIncludeFilesRecurse
//------------------------------------------------------------------------------
void Report::GetIncludeFilesRecurse(IncludeStatsMap& incStats, const Node* node) const
{
    const Node::Type type = node->GetType();
    if ( type == Node::OBJECT_NODE )
    {
        // Dynamic Deps
        const Dependencies& dynamicDeps = node->GetDynamicDependencies();
        const Dependency* const end = dynamicDeps.End();
        for ( const Dependency* it = dynamicDeps.Begin(); it != end; ++it )
        {
            AddInclude( incStats, it->GetNode(), node );
        }

        return;
    }

    // Static Deps
    const Dependencies& staticDeps = node->GetStaticDependencies();
    const Dependency* end = staticDeps.End();
    for ( const Dependency* it = staticDeps.Begin(); it != end; ++it )
    {
        GetIncludeFilesRecurse( incStats, it->GetNode() );
    }

    // Dynamic Deps
    const Dependencies& dynamicDeps = node->GetDynamicDependencies();
    end = dynamicDeps.End();
    for ( const Dependency* it = dynamicDeps.Begin(); it != end; ++it )
    {
        GetIncludeFilesRecurse( incStats, it->GetNode() );
    }
}

// AddInclude
//------------------------------------------------------------------------------
void Report::AddInclude(IncludeStatsMap& incStats, const Node* node, const Node* parentNode) const
{
    bool isHeaderInPCH = false;
    if ( parentNode->GetType() == Node::OBJECT_NODE )
    {
        const ObjectNode* obj = parentNode->CastTo< ObjectNode >();
        isHeaderInPCH = obj->IsCreatingPCH();
    }

    // check for existing
    IncludeStats* stats = incStats.Find( node );
    if ( stats == nullptr )
    {
        stats = incStats.Insert( node );
    }

    stats->count++;
    stats->inPCH |= isHeaderInPCH;
}


// IncludeStatsMap (CONSTRUCTOR)
//------------------------------------------------------------------------------
Report::IncludeStatsMap::IncludeStatsMap()
    : m_Pool(sizeof(IncludeStats), __alignof(IncludeStats))
{
    memset( m_Table, 0, sizeof(m_Table) );
}

// IncludeStatsMap (DESTRUCTOR)
//------------------------------------------------------------------------------
Report::IncludeStatsMap::~IncludeStatsMap()
{
    for (size_t i = 0; i < 65536; ++i)
    {
        IncludeStats* item = m_Table[i];
        while (item)
        {
            IncludeStats* next = item->m_Next;
            m_Pool.Free(item);
            item = next;
        }
    }
}

// Find
//------------------------------------------------------------------------------
Report::IncludeStats* Report::IncludeStatsMap::Find(const Node* node) const
{
    // caculate table entry
    const uint32_t hash = node->GetNameCRC();
    const uint32_t key = ( hash & 0xFFFF );
    IncludeStats* item = m_Table[key];

    // check linked list
    while (item)
    {
        if (item->node == node)
        {
            return item;
        }
        item = item->m_Next;
    }

    // not found
    return nullptr;
}

// Insert
//------------------------------------------------------------------------------
Report::IncludeStats* Report::IncludeStatsMap::Insert(const Node* node)
{
    // caculate table entry
    const uint32_t hash = node->GetNameCRC();
    const uint32_t key = ( hash & 0xFFFF );

    // insert new item
    IncludeStats* newStats = (IncludeStats*)m_Pool.Alloc();
    newStats->node = node;
    newStats->count = 0;
    newStats->inPCH = false;
    newStats->m_Next = m_Table[key];
    m_Table[key] = newStats;

    return newStats;
}

// Flatten
//------------------------------------------------------------------------------
void Report::IncludeStatsMap::Flatten(Array< const IncludeStats* >& stats) const
{
    for (size_t i = 0; i < 65536; ++i)
    {
        IncludeStats* item = m_Table[i];
        while ( item )
        {
            IncludeStats* next = item->m_Next;
            stats.Append( item );
            item = next;
        }
    }
}

//------------------------------------------------------------------------------
