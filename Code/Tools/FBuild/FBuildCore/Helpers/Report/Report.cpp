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
#include "Tools/FBuild/FBuildCore/Helpers/Report/HTMLReport.h"
#include "Tools/FBuild/FBuildCore/Helpers/Report/JSONReport.h"

// Core
#include "Core/Strings/AStackString.h"

// system
#include <stdarg.h> // for va_args
#include <string.h>
#include <time.h>

//------------------------------------------------------------------------------
/*static*/ void Report::Generate( const AString & reportType,
                                  const NodeGraph & nodeGraph,
                                  const FBuildStats & stats )
{
    if ( reportType == "json" )
    {
        JSONReport report;
        report.Generate( nodeGraph, stats );
        report.Save();
    }
    else
    {
        ASSERT( reportType == "html" );
        HTMLReport report;
        report.Generate( nodeGraph, stats );
        report.Save();
    }
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
Report::Report()
    : m_LibraryStats( 512, true )
{
    // pre-allocate a large string for output
    m_Output.SetReserved( MEGABYTE );
    m_Output.SetLength( 0 );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Report::~Report()
{
    for ( const LibraryStats * ls : m_LibraryStats )
    {
        FDELETE ls;
    }
}

// Write
//------------------------------------------------------------------------------
void Report::Write( MSVC_SAL_PRINTF const char * fmtString, ... )
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

// GetReportDateTime
//------------------------------------------------------------------------------
void Report::GetReportDateTime( AString & outReportDateTime ) const
{
    // report time
    time_t rawtime;
    struct tm * timeinfo;
    time( &rawtime );
    PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'localtime' is deprecated: This function or variable may be unsafe...
    timeinfo = localtime( &rawtime ); // TODO:C Consider using localtime_s
    PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
    PRAGMA_DISABLE_POP_MSVC // 4996
    char timeBuffer[ 256 ];
    // Mon 1-Jan-2000 - 18:01:15
    VERIFY( strftime( timeBuffer, 256, "%a %d-%b-%Y - %H:%M:%S", timeinfo ) > 0 );
    outReportDateTime = timeBuffer;
}

// FixupTimeTakenPlaceholder
//------------------------------------------------------------------------------
void Report::FixupTimeTakenPlaceholder()
{
    const float timeTakenSecs =  m_Timer.GetElapsed();

    // patch in time take
    AStackString<> timeTakenBuffer;
    FBuildStats::FormatTime( timeTakenSecs, timeTakenBuffer );
    m_Output.Replace( GetTimeTakenPlaceholder(), timeTakenBuffer.Get() );
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
void Report::GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Node * node, LibraryStats * currentLib ) const
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

        currentLib->m_ObjectCount++;

        const bool cacheHit = node->GetStatFlag( Node::STATS_CACHE_HIT );
        const bool cacheMiss = node->GetStatFlag( Node::STATS_CACHE_MISS );
        if ( cacheHit || cacheMiss )
        {
            currentLib->m_ObjectCount_Cacheable++;

            if ( cacheHit )
            {
                currentLib->m_ObjectCount_CacheHits++;
            }
            if ( node->GetStatFlag( Node::STATS_CACHE_STORE ) )
            {
                currentLib->m_ObjectCount_CacheStores++;
                currentLib->m_CacheTimeMS += node->GetCachingTime();
            }
        }

        if ( cacheHit || cacheMiss || node->GetStatFlag( Node::STATS_BUILT ) )
        {
            currentLib->m_ObjectCount_OutOfDate++;
            currentLib->m_CPUTimeMS += node->GetProcessingTime();
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
        currentLib->m_Library = node;
        currentLib->m_CPUTimeMS = 0;
        currentLib->m_ObjectCount = 0;
        currentLib->m_ObjectCount_OutOfDate = 0;
        currentLib->m_ObjectCount_Cacheable = 0;
        currentLib->m_ObjectCount_CacheHits = 0;
        currentLib->m_ObjectCount_CacheStores = 0;
        currentLib->m_CacheTimeMS = 0;

        // count time for library/dll itself
        if ( node->GetStatFlag( Node::STATS_BUILT ) || node->GetStatFlag( Node::STATS_FAILED ) )
        {
            currentLib->m_CPUTimeMS += node->GetProcessingTime();
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
void Report::GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Dependencies & dependencies, LibraryStats * currentLib ) const
{
    for ( const Dependency & dep : dependencies )
    {
        GetLibraryStatsRecurse( libStats, dep.GetNode(), currentLib );
    }
}

// GetIncludeFilesRecurse
//------------------------------------------------------------------------------
void Report::GetIncludeFilesRecurse( IncludeStatsMap & incStats, const Node * node ) const
{
    const Node::Type type = node->GetType();
    if ( type == Node::OBJECT_NODE )
    {
        // Dynamic Deps
        for ( const Dependency & dep : node->GetDynamicDependencies() )
        {
            AddInclude( incStats, dep.GetNode(), node );
        }

        return;
    }

    // Static Deps
    for ( const Dependency & dep : node->GetStaticDependencies() )
    {
        GetIncludeFilesRecurse( incStats, dep.GetNode() );
    }

    // Dynamic Deps
    for ( const Dependency & dep : node->GetDynamicDependencies() )
    {
        GetIncludeFilesRecurse( incStats, dep.GetNode() );
    }
}

// AddInclude
//------------------------------------------------------------------------------
void Report::AddInclude( IncludeStatsMap & incStats, const Node * node, const Node * parentNode ) const
{
    bool isHeaderInPCH = false;
    if ( parentNode->GetType() == Node::OBJECT_NODE )
    {
        const ObjectNode * obj = parentNode->CastTo< ObjectNode >();
        isHeaderInPCH = obj->IsCreatingPCH();
    }

    // check for existing
    IncludeStats * stats = incStats.Find( node );
    if ( stats == nullptr )
    {
        stats = incStats.Insert( node );
    }

    stats->m_Count++;
    stats->m_InPCH |= isHeaderInPCH;
}

// IncludeStatsMap (CONSTRUCTOR)
//------------------------------------------------------------------------------
Report::IncludeStatsMap::IncludeStatsMap()
    : m_Pool( sizeof( IncludeStats ), __alignof( IncludeStats ) )
{
    memset( m_Table, 0, sizeof( m_Table ) );
}

// IncludeStatsMap (DESTRUCTOR)
//------------------------------------------------------------------------------
Report::IncludeStatsMap::~IncludeStatsMap()
{
    for ( size_t i = 0; i < 65536; ++i )
    {
        IncludeStats * item = m_Table[ i ];
        while ( item )
        {
            IncludeStats * next = item->m_Next;
            m_Pool.Free( item );
            item = next;
        }
    }
}

// Find
//------------------------------------------------------------------------------
Report::IncludeStats * Report::IncludeStatsMap::Find( const Node * node ) const
{
    // caculate table entry
    const uint32_t hash = node->GetNameCRC();
    const uint32_t key = ( hash & 0xFFFF );
    IncludeStats * item = m_Table[ key ];

    // check linked list
    while ( item )
    {
        if ( item->m_Node == node )
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
Report::IncludeStats * Report::IncludeStatsMap::Insert( const Node * node )
{
    // caculate table entry
    const uint32_t hash = node->GetNameCRC();
    const uint32_t key = ( hash & 0xFFFF );

    // insert new item
    IncludeStats * newStats = (IncludeStats *)m_Pool.Alloc();
    newStats->m_Node = node;
    newStats->m_Count = 0;
    newStats->m_InPCH = false;
    newStats->m_Next = m_Table[ key ];
    m_Table[ key ] = newStats;

    return newStats;
}

// Flatten
//------------------------------------------------------------------------------
void Report::IncludeStatsMap::Flatten( Array< const IncludeStats * > & stats ) const
{
    for ( size_t i = 0; i < 65536; ++i )
    {
        IncludeStats * item = m_Table[ i ];
        while ( item )
        {
            IncludeStats * next = item->m_Next;
            stats.Append( item );
            item = next;
        }
    }
}

//------------------------------------------------------------------------------
