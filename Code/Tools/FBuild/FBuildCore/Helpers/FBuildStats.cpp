// FBuildStats
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FBuildStats.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Helpers/Report.h"

// Core
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// NodeCostSorter
//------------------------------------------------------------------------------
class NodeCostSorter
{
public:
	inline bool operator () ( const Node * a, const Node * b ) const
	{
		return ( a->GetProcessingTime() > b->GetProcessingTime() );
	}
};

// CONSTRUCTOR - FBuildStats
//------------------------------------------------------------------------------
FBuildStats::FBuildStats()
	: m_NodeTimeTotalms( 0 )
	, m_NodeTimeProgressms( 0 )
	, m_TotalBuildTime( 0.0f )
	, m_TotalLocalCPUTimeMS( 0 )
	, m_TotalRemoteCPUTimeMS( 0 )
	, m_RootNode( nullptr )
	, m_NodesByTime( 100 * 1000, true )
{}

// CONSTRUCTOR - FBuildStats::Stats
//------------------------------------------------------------------------------
FBuildStats::Stats::Stats()
	: m_NumProcessed( 0 )
	, m_NumBuilt( 0 )
	, m_NumCacheHits( 0 )
	, m_NumCacheMisses( 0 )
	, m_NumCacheStores( 0 )
	, m_ProcessingTimeMS( 0 )
{}

// OnBuildStop
//------------------------------------------------------------------------------
void FBuildStats::OnBuildStop( Node * node )
{
	m_RootNode = node;

	const FBuildOptions & options = FBuild::Get().GetOptions();
	const bool showSummary = options.m_ShowSummary;
	const bool generateReport = options.m_GenerateReport;

	// Any output required?
	if ( showSummary || generateReport )
	{
		// do work common to -summary and -report
		GatherPostBuildStatistics( node );

		// detailed build report
		if ( generateReport )
		{
			Report r;
			r.Generate( *this );
			r.Save();
		}

		// stdout summary
		if ( showSummary )
		{
			OutputSummary();
		}
	}
}

// GatherPostBuildStatistics
//------------------------------------------------------------------------------
void FBuildStats::GatherPostBuildStatistics( Node * node )
{
    PROFILE_FUNCTION

	// recurse and gather the per-node-type statistics
	GatherPostBuildStatisticsRecurse( node );

	NodeCostSorter ncs;
	m_NodesByTime.Sort( ncs );

	// Total the stats
	for ( uint32_t i=0; i< Node::NUM_NODE_TYPES; ++i )
	{
		m_Totals.m_NumProcessed		+= m_PerTypeStats[ i ].m_NumProcessed;
		m_Totals.m_NumBuilt			+= m_PerTypeStats[ i ].m_NumBuilt;
		m_Totals.m_NumCacheHits		+= m_PerTypeStats[ i ].m_NumCacheHits;
		m_Totals.m_NumCacheMisses	+= m_PerTypeStats[ i ].m_NumCacheMisses;
		m_Totals.m_NumCacheStores	+= m_PerTypeStats[ i ].m_NumCacheStores;
	}
}

// OutputSummary
//------------------------------------------------------------------------------
void FBuildStats::OutputSummary() const
{
    PROFILE_FUNCTION

    AStackString< 2048 > output;

	// Top 10 cost items
	if ( m_NodesByTime.IsEmpty() == false )
	{
        output += "--- Most Expensive ----------------------------------------------\n";
		output += "Time (s)  Name:\n";
		size_t itemsToDisplay = Math::Min( m_NodesByTime.GetSize(), (size_t)20 );
		for ( size_t i=0; i<itemsToDisplay; ++i )
		{
			const Node * n = m_NodesByTime[ i ];
			output.AppendFormat( "%-9.3f %s\n", ( (float)n->GetProcessingTime() / 1000.0f ), n->GetName().Get() );
		}
		output += "\n";
	}

	output += "--- Summary -----------------------------------------------------\n";

	// Per-Node type stats
	// NOTE: Only showing the interesting nodes
	output += "                                 /----- Cache -----\\\n";
	output += "Build:          Seen    Built   Hit     Miss    Store   CPU\n";
	for ( uint32_t i=0; i< Node::NUM_NODE_TYPES; ++i )
	{
		// don't show nodes with no task
		const Stats & stats = m_PerTypeStats[ i ];
		if ( stats.m_NumProcessed == 0 )
		{
			continue;
		}

		const char * typeName = Node::GetTypeName( Node::Type( i ) );

		AStackString<> cpuTime;
		FormatTime( (float)( (double)stats.m_ProcessingTimeMS / (double)1000 ), cpuTime );

		AStackString<> cacheInfo;
		if ( ( stats.m_NumCacheHits + stats.m_NumCacheMisses + stats.m_NumCacheStores ) > 0 )
		{
			cacheInfo.Format( "%-8u%-8u%-8u",
							  stats.m_NumCacheHits,
							  stats.m_NumCacheMisses,
							  stats.m_NumCacheStores );
		}
		else
		{
			cacheInfo = "-       -       -       ";
		}
		output.AppendFormat( " - %-10s : %-8u%-8u%s%s\n",
						typeName,
						stats.m_NumProcessed,
						stats.m_NumBuilt,
						cacheInfo.Get(),
						cpuTime.Get() );
	}
	output += "Cache:\n";
	{
		const uint32_t hits = m_Totals.m_NumCacheHits;
		const uint32_t misses = m_Totals.m_NumCacheMisses;
		const uint32_t stores = m_Totals.m_NumCacheStores;
		float hitPerc = 0.0f;
		if ( hits > 0 || misses > 0 )
		{
			hitPerc = ( (float)hits / float( hits + misses ) * 100.0f );
		}
		output.AppendFormat( " - Hits       : %u (%2.1f %%)\n", hits, hitPerc );
		output.AppendFormat( " - Misses     : %u\n", misses );
		output.AppendFormat( " - Stores     : %u\n", stores );
	}

	AStackString<> buffer;
	FormatTime( m_TotalBuildTime, buffer );
	output += "Time:\n";
	output.AppendFormat( " - Real       : %s\n", buffer.Get() );
	float totalLocalCPUInSeconds = (float)( (double)m_TotalLocalCPUTimeMS / (double)1000 );
	float totalRemoteCPUInSeconds = (float)( (double)m_TotalRemoteCPUTimeMS / (double)1000 );
	FormatTime( totalLocalCPUInSeconds, buffer );
	float localRatio = ( totalLocalCPUInSeconds / m_TotalBuildTime );
	output.AppendFormat( " - Local CPU  : %s (%2.1f:1)\n", buffer.Get(), localRatio );
	FormatTime( totalRemoteCPUInSeconds, buffer );
	float remoteRatio = ( totalRemoteCPUInSeconds / m_TotalBuildTime );
	output.AppendFormat( " - Remote CPU : %s (%2.1f:1)\n", buffer.Get(), remoteRatio );
	output += "-----------------------------------------------------------------\n";

	OUTPUT( "%s", output.Get() );
}

// GatherPostBuildStatisticsRecurse
//------------------------------------------------------------------------------
void FBuildStats::GatherPostBuildStatisticsRecurse( Node * node )
{
	// have we seen this node when gathering stats?
	if ( node->GetStatFlag( Node::STATS_STATS_PROCESSED ) )
	{
		return;
	}

	Node::Type nodeType = node->GetType();
	Stats & stats = m_PerTypeStats[ nodeType ];

	if ( node->GetType() != Node::PROXY_NODE )
	{
		stats.m_NumProcessed++;

		m_TotalLocalCPUTimeMS += node->GetProcessingTime();
		if (node->GetStatFlag(Node::STATS_BUILT_REMOTE))
		{
			m_TotalRemoteCPUTimeMS += node->GetLastBuildTime();
		}
		stats.m_ProcessingTimeMS += node->GetProcessingTime();

		// add our node
		if ( node->GetProcessingTime() > 0 )
		{
			// don't add filenodes (too spammy)
			if ( node->GetType() != Node::FILE_NODE )
			{
				m_NodesByTime.Append( node );
			}
		}

		if ( node->GetStatFlag( Node::STATS_BUILT ) )
		{
			stats.m_NumBuilt++;
		}
		if ( node->GetStatFlag( Node::STATS_CACHE_HIT ) )
		{
			stats.m_NumCacheHits++;
		}
		if ( node->GetStatFlag( Node::STATS_CACHE_MISS ) )
		{
			stats.m_NumCacheMisses++;
		}
		if ( node->GetStatFlag( Node::STATS_CACHE_STORE ) )
		{
			stats.m_NumCacheStores++;
		}
	}

	// mark this node as processed to prevent multiple recursion
	node->SetStatFlag( Node::STATS_STATS_PROCESSED );

	// handle deps
	GatherPostBuildStatisticsRecurse( node->GetPreBuildDependencies() );
	GatherPostBuildStatisticsRecurse( node->GetStaticDependencies() );
	GatherPostBuildStatisticsRecurse( node->GetDynamicDependencies() );
}

// GatherPostBuildStatisticsRecurse
//------------------------------------------------------------------------------
void FBuildStats::GatherPostBuildStatisticsRecurse( const Dependencies & dependencies )
{
	const Dependencies::Iter end = dependencies.End();
	for ( Dependencies::Iter it = dependencies.Begin();
		  it != end;
		  it++ )
	{
		GatherPostBuildStatisticsRecurse( it->GetNode() );
	}
}

// FormatTime
//------------------------------------------------------------------------------
void FBuildStats::FormatTime( float timeInSeconds , AString & buffer ) const
{
	buffer.Clear();

	uint32_t days = (uint32_t)( timeInSeconds / ( 24.0f * 60.0f * 60.0f ) );
	timeInSeconds -= ( (float)days * ( 24.0f * 60.0f * 60.0f ) );
	uint32_t hours = (uint32_t)( timeInSeconds / ( 60.0f * 60.0f ) );
	timeInSeconds -= ( (float)hours * ( 60.0f * 60.0f ) );
	uint32_t mins = (uint32_t)( timeInSeconds / 60.0f );
	timeInSeconds -= ( (float)mins * 60.0f );

	AStackString<> temp;

	if ( days > 0 )
	{
		temp.Format( "%u days, ", days );
		buffer += temp;
	}
	if ( hours > 0 )
	{
		temp.Format( "%uh:", hours );
		buffer += temp;
	}
	if ( mins > 0 )
	{
		temp.Format( "%um ", mins );
		buffer += temp;
	}

	temp.Format( "%2.3fs", timeInSeconds );
	buffer += temp;
}

//------------------------------------------------------------------------------
