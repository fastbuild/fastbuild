// JSONReport
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "JSONReport.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/FBuildStats.h"
#include "Tools/FBuild/FBuildCore/Helpers/JSON.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
JSONReport::JSONReport() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
JSONReport::~JSONReport() = default;

// Generate
//------------------------------------------------------------------------------
void JSONReport::Generate( const NodeGraph & nodeGraph, const FBuildStats & stats )
{
    GetLibraryStats( nodeGraph, stats );
    
    Write( "{\n\t" );

    // build the report
    CreateOverview( stats );
    Write( ",\n\t" );

    DoCPUTimeByType( stats );
    Write( ",\n\t" );

    DoCacheStats( stats );
    Write( ",\n\t" );

    DoCPUTimeByLibrary();
    Write( ",\n\t" );

    DoCPUTimeByItem( stats );
    Write( ",\n\t" );

    DoIncludes();
    Write( "\n}" );

    // patch in time take
    FixupTimeTakenPlaceholder();
}

// Save
//------------------------------------------------------------------------------
void JSONReport::Save() const
{
    FileStream f;
    if ( f.Open( "report.json", FileStream::WRITE_ONLY ) )
    {
        f.Write( m_Output.Get(), m_Output.GetLength() );
    }
}

// CreateOverview
//------------------------------------------------------------------------------
void JSONReport::CreateOverview( const FBuildStats & stats )
{
    AStackString<> buffer;

    Write( "\"Overview\": {\n" );
    Write( "\t\t" );

    // Full command line
    AStackString<> commandLineBuffer;
    Env::GetCmdLine( commandLineBuffer );
    #if defined( __WINDOWS__ )
        const char * exeExtension = commandLineBuffer.FindLast( ".exe\"" );
        const char * commandLine = exeExtension ? ( exeExtension + 5 ) : commandLineBuffer.Get(); // skip .exe + closing quote
    #else
        const char * commandLine = commandLineBuffer.Get();
    #endif

    AStackString<> programName( commandLine );
    JSON::Escape( programName );
    Write( "\"cmd line options\": \"%s\",\n\t\t", programName.Get() );

    // Target
    AStackString<> targets;
    const Node * rootNode = stats.GetRootNode();
    if ( rootNode->GetType() != Node::PROXY_NODE )
    {
        targets = rootNode->GetName();
    }
    else
    {
        const Dependencies & childNodes = rootNode->GetStaticDependencies();
        const size_t num = childNodes.GetSize();
        for ( size_t i=0; i<num; ++i )
        {
            if ( i != 0 )
            {
                targets += ", ";
            }
            const Node * child = childNodes[ i ].GetNode();
            targets += child->GetName();
        }
    }

    Write( "\"Target(s)\": \"%s\",\n\t\t", targets.Get() );

    // Result
    const bool buildOK = ( stats.GetRootNode()->GetState() == Node::UP_TO_DATE );
    Write( "\"Result\": \"%s\",\n\t\t", buildOK ? "OK" : "FAILED" );

    // Real Time
    const float totalBuildTime = stats.m_TotalBuildTime;
    stats.FormatTime( totalBuildTime, buffer );
    Write( "\"Time\": \"%s\",\n\t\t", buffer.Get() );

    // Local CPU Time
    const float totalLocalCPUInSeconds = (float)( (double)stats.m_TotalLocalCPUTimeMS / (double)1000 );
    stats.FormatTime( totalLocalCPUInSeconds, buffer );
    const float localRatio = ( totalLocalCPUInSeconds / totalBuildTime );
    Write( "\"CPU Time\": \"%s (%.1f:1)\",\n\t\t", buffer.Get(), (double)localRatio );

    // Remote CPU Time
    const float totalRemoteCPUInSeconds = (float)( (double)stats.m_TotalRemoteCPUTimeMS / (double)1000 );
    stats.FormatTime( totalRemoteCPUInSeconds, buffer );
    const float remoteRatio = ( totalRemoteCPUInSeconds / totalBuildTime );
    Write( "\"Remote CPU Time\": \"%s (%.1f:1)\",\n\t\t", buffer.Get(), (double)remoteRatio );

    // version info
    Write( "\"Version\": \"%s %s\",\n\t\t", FBUILD_VERSION_STRING, FBUILD_VERSION_PLATFORM );

    // report time
    AStackString<> reportDateTime;
    GetReportDateTime( reportDateTime );

    // NOTE: leave space to patch in time taken later
    Write( "\"Report Generated\": \"%s - %s\"\n\t", GetTimeTakenPlaceholder(), reportDateTime.Get() );
    Write( "}" );
}

// DoCPUTimeByType
//------------------------------------------------------------------------------
void JSONReport::DoCPUTimeByType(const FBuildStats& stats)
{
    Write( "\"CPU Time by Node Type\": {" );
    Write( "\n\t\t" );

    Array< TimingStats > items(32, true);

    for ( size_t i = 0; i < (size_t)Node::NUM_NODE_TYPES; ++i )
    {
        const FBuildStats::Stats& nodeStats = stats.GetStatsFor( (Node::Type)i );
        if ( nodeStats.m_NumProcessed == 0 )
        {
            continue;
        }

        // label
        const char* typeName = Node::GetTypeName( Node::Type(i) );
        const float value = (float)( (double)nodeStats.m_ProcessingTimeMS / (double)1000 );

        items.EmplaceBack( typeName, value, (void *)i );
    }

    items.Sort();

    // calculate total time taken for all node types
    float total = 0.0f;
    for ( size_t i = 0; i < items.GetSize(); ++i )
    {
        total += items[ i ].m_Value;
    }

    AStackString<> buffer;
    for ( size_t i = 0; i < items.GetSize(); ++i )
    {
        const Node::Type type = (Node::Type)(size_t)items[ i ].m_UserData;
        const FBuildStats::Stats& nodeStats = stats.GetStatsFor( type );
        if ( nodeStats.m_NumProcessed == 0 )
        {
            continue;
        }

        const char* typeName = Node::GetTypeName( type );
        const float value = (float)( (double)nodeStats.m_ProcessingTimeMS / (double)1000 );
        const uint32_t processed = nodeStats.m_NumProcessed;
        const uint32_t built = nodeStats.m_NumBuilt;
        const uint32_t cacheHits = nodeStats.m_NumCacheHits;

        Write( "\"%s\": {", typeName );
        Write( "\n\t\t\t" );

        Write( "\"Time (s)\": %.3f,\n\t\t\t", (double)value );
        Write( "\"Processed\": %u,\n\t\t\t", processed );
        Write( "\"Built\": %u,\n\t\t\t", built );

        if ( type == Node::OBJECT_NODE )
        {
            // cacheable
            Write( "\"Cache Hits\": %u,\n\t\t\t", cacheHits );
        }
        else
        {
            // non-cacheable
            Write( "\"Cache Hits\": \"-\",\n\t\t\t" );
        }

        const float percent = ( items[ i ].m_Value / total ) * 100.0f;
        Write( "\"Percentage\": %.1f", (double)percent );

        Write( "\n\t\t" );
        Write( "}" );

        if ( i < items.GetSize() - 1 )
        {
            Write( ",\n\t\t" );
        }
    }

    Write( "\n\t}" );
}

// DoCacheStats
//------------------------------------------------------------------------------
void JSONReport::DoCacheStats( const FBuildStats & /*stats*/ )
{
    Write( "\"Cache Stats\": {\n" );

    const FBuildOptions & options = FBuild::Get().GetOptions();
    if ( options.m_UseCacheRead || options.m_UseCacheWrite )
    {
        // avoid writing useless table
        uint32_t totalOutOfDateItems( 0 );
        uint32_t totalCacheable( 0 );
        uint32_t totalCacheHits( 0 );
        for ( const LibraryStats * ls : m_LibraryStats )
        {
            totalOutOfDateItems += ls->m_ObjectCount_OutOfDate;
            totalCacheable += ls->m_ObjectCount_Cacheable;
            totalCacheHits += ls->m_ObjectCount_CacheHits;
        }
        if ( totalOutOfDateItems == 0 )
        {
            Write( "\t}" );
            return;
        }
        const uint32_t totalCacheMisses( totalCacheable - totalCacheHits );

        Write( "\t\t" );
        Write( "\"summary\": {\n\t\t\t" );

        Array< TimingStats > items( 3, false );
        items.EmplaceBack( "Uncacheable", (float)(totalOutOfDateItems - totalCacheable) );
        items.EmplaceBack( "Cache Miss", (float)totalCacheMisses );
        items.EmplaceBack( "Cache Hit", (float)totalCacheHits );

        AStackString<> buffer;
        for ( size_t i = 0; i < items.GetSize(); ++i )
        {
            const float percent = ( items[ i ].m_Value / (float)totalOutOfDateItems ) * 100.0f;

            Write( "\"%s\": {", items[ i ].m_Label );
            Write( "\n\t\t\t\t" );
            Write( "\"Time (s)\": %.3f,", (double)items[ i ].m_Value );
            Write( "\n\t\t\t\t" );
            Write( "\"Percentage\": %.1f", (double)percent );
            Write( "\n\t\t\t" );
            Write( "}" );

            // add a comma and new line as long as we're not the last item
            if ( i < items.GetSize() - 1 )
            {
                Write( ",\n\t\t\t" );
            }
        }

        // end of summary section
        Write( "\n\t\t}," );

        // library stats information
        Write( "\n\t\t\"details\": [\n\t\t\t" );

        size_t numOutput( 0 );

        // items
        for ( const LibraryStats * ls : m_LibraryStats )
        {
            const char * libraryName = ls->m_Library->GetName().Get();

            // total items in library
            const uint32_t totalItemsCount = ls->m_ObjectCount;

            // out of date items
            const uint32_t  outOfDateItems = ls->m_ObjectCount_OutOfDate;
            if ( outOfDateItems == 0 )
            {
                continue; // skip library if nothing was done
            }
            const float     outOfDateItemsPerc = ( (float)outOfDateItems / (float)totalItemsCount) * 100.0f;

            // cacheable
            const uint32_t  cItems      = ls->m_ObjectCount_Cacheable;
            const float     cItemsPerc  = ( (float)cItems / (float)outOfDateItems ) * 100.0f;

            // hits
            const uint32_t  cHits       = ls->m_ObjectCount_CacheHits;
            const float     cHitsPerc   = ( cItems > 0 ) ? ( (float)cHits / (float)cItems ) * 100.0f : 0.0f;

            // misses
            const uint32_t  cMisses     = ( cItems - cHits );
            const float     cMissesPerc = ( cMisses > 0 ) ? 100.0f - cHitsPerc : 0.0f;

            // stores
            const uint32_t  cStores     = ls->m_ObjectCount_CacheStores;
            const float     cStoreTime  = (float)ls->m_CacheTimeMS / 1000.0f; // ms to s

            if ( numOutput > 0 )
            {
                Write( ",\n\t\t\t" );
            }

            Write( "{" );
            Write( "\n\t\t\t\t" );

            Write( "\"Library\": \"%s\",\n\t\t\t\t", libraryName );
            Write( "\"Items\": %u,\n\t\t\t\t", totalItemsCount);
            Write( "\"Out-of-Date\": {" );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Count\": %u,", outOfDateItems );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Percentage\": %.1f", (double)outOfDateItemsPerc );
            Write( "\n\t\t\t\t" );
            Write( "}," );
            Write( "\n\t\t\t\t" );

            Write( "\"Cacheable\": {" );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Count\": %u,", cItems );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Percentage\": %.1f", (double)cItemsPerc );
            Write( "\n\t\t\t\t" );
            Write( "}," );
            Write( "\n\t\t\t\t" );

            Write( "\"Hits\": {" );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Count\": %u,", cHits );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Percentage\": %.1f", (double)cHitsPerc );
            Write( "\n\t\t\t\t" );
            Write( "}," );
            Write( "\n\t\t\t\t" );

            Write( "\"Misses\": {" );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Count\": %u,", cMisses );
            Write( "\n\t\t\t\t\t" );
            Write( "\"Percentage\": %.1f", (double)cMissesPerc );
            Write( "\n\t\t\t\t" );
            Write( "}," );
            Write( "\n\t\t\t\t" );

            Write( "\"Stores\": %u,\n\t\t\t\t", cStores );
            Write( "\"Store Time (s)\": %.3f\n\t\t\t", (double)cStoreTime );

            Write( "}" );

            numOutput++;
        }
    }
    else
    {
        Write( "\t}" );
    }

    // end library stats
    Write( "\n\t\t ]" );
    Write( "\n\t}" );
}

// DoCPUTimeByLibrary
//------------------------------------------------------------------------------
void JSONReport::DoCPUTimeByLibrary()
{
    Write( "\"CPU Time by Library\": [" );

    // total
    uint32_t total = 0;
    for ( const LibraryStats * ls : m_LibraryStats )
    {
        total += ls->m_CPUTimeMS;
    }
    if ( total == 0 )
    {
        Write( "]" );
        return;
    }

    Write( "\n\t\t" );

    const float totalS = (float)((double)total * 0.001);
    size_t numOutput( 0 );
    // Result
    for ( const LibraryStats * ls : m_LibraryStats )
    {
        if ( ls->m_CPUTimeMS == 0 )
        {
            continue;
        }

        const uint32_t objCount = ls->m_ObjectCount_OutOfDate;
        const float time = ( (float)ls->m_CPUTimeMS * 0.001f ); // ms to s
        const float perc = (float)( (double)time / (double)totalS * 100 );
        const char* type = ls->m_Library->GetTypeName();
        switch ( ls->m_Library->GetType() )
        {
            case Node::LIBRARY_NODE:        type = "Static"; break;
            case Node::DLL_NODE:            type = "DLL"; break;
            case Node::CS_NODE:             type = "C# DLL"; break;
            case Node::OBJECT_LIST_NODE:    type = "ObjectList"; break;
            default:                        break;
        }
        const char* name = ls->m_Library->GetName().Get();

        Write( "{" );
        Write( "\n\t\t\t" );

        Write( "\"Time (s)\": %.3f,\n\t\t\t", (double)time );
        Write( "\"Percentage\": %.1f,\n\t\t\t", (double)perc );
        Write( "\"Obj Built\": %u,\n\t\t\t", objCount );
        Write( "\"Type\": \"%s\",\n\t\t\t", type );

        AStackString<> itemName( name );
        JSON::Escape( itemName );
        Write( "\"Name\": \"%s\"\n\t\t", itemName.Get() );

        Write( "}" );

        if ( numOutput < m_LibraryStats.GetSize() - 1 )
        {
            Write( ",\n\t\t" );
        }

        numOutput++;
    }

    // end CPU Time by Library array
    Write( "\n\t ]" );
}

// DoCPUTimeByItem
//------------------------------------------------------------------------------
void JSONReport::DoCPUTimeByItem( const FBuildStats & stats )
{
    const FBuildOptions & options = FBuild::Get().GetOptions();
    const bool cacheEnabled = ( options.m_UseCacheRead || options.m_UseCacheWrite );

    Write( "\"CPU Time by Item\": [\n" );
    Write( "\t\t" );

    size_t numOutput = 0;

    // Result
    const Array< const Node * > & nodes = stats.GetNodesByTime();
    for ( const Node ** it = nodes.Begin();
          it != nodes.End();
          ++ it )
    {
        const Node * node = *it;
        const float time = ( (float)node->GetProcessingTime() * 0.001f ); // ms to s
        const char * type = node->GetTypeName();
        const char * name = node->GetName().Get();

        if ( cacheEnabled )
        {
            const bool cacheHit = node->GetStatFlag( Node::STATS_CACHE_HIT );
            const bool cacheStore = node->GetStatFlag( Node::STATS_CACHE_STORE );

            Write( "{" );
            Write( "\n\t\t\t" );

            Write( "\"Time (s)\": %2.3f,\n\t\t\t", (double)time );
            Write( "\"Type\": \"%s\",\n\t\t\t", type );
            Write( "\"Cache\": \"%s\",\n\t\t\t", cacheHit ? "HIT" : (cacheStore ? "STORE" : "N/A") );

            AStackString<> itemName( name );
            JSON::Escape( itemName );
            Write( "\"Name\": \"%s\"\n\t\t", itemName.Get() );

            Write( "}" );
        }
        else
        {
            Write( "{" );
            Write( "\n\t\t\t" );

            Write( "\"Time\": \"%2.3fs\",\n\t\t\t", (double)time );
            Write( "\"Type\": \"%s\",\n\t\t\t", type );

            AStackString<> itemName( name );
            JSON::Escape( itemName );
            Write( "\"Name\": \"%s\"\n\t\t", itemName.Get() );

            Write( "}" );
        }

        if ( numOutput < nodes.GetSize() - 1 )
        {
            Write( ",\n\t\t" );
        }

        numOutput++;
    }

    Write( "\n\t ]" );
}

// DoIncludes
//------------------------------------------------------------------------------
PRAGMA_DISABLE_PUSH_MSVC( 6262 ) // warning C6262: Function uses '262212' bytes of stack
void JSONReport::DoIncludes()
{
    Write( "\"Includes\": [" );
    Write( "\n\t\t" );

    size_t numLibsOutput = 0;

    // build per-library stats
    for ( const LibraryStats * ls : m_LibraryStats )
    {
        if ( ls->m_ObjectCount_OutOfDate == 0 )
        {
            continue;
        }

        // get all the includes for this library
        const Node * library = ls->m_Library;
        IncludeStatsMap incStatsMap;
        GetIncludeFilesRecurse( incStatsMap, library );

        // flatten and sort by usage
        Array< const IncludeStats * > incStats( 10 * 1024, true );
        incStatsMap.Flatten( incStats );
        incStats.SortDeref();

        Write( "{\n\t\t\t" );
        Write( "\"Library name\": \"%s\",", library->GetName().Get() );
        Write( "\n\t\t\t" );
        Write( "\"Includes\": [" );

        if ( incStats.GetSize() == 0 )
        {
            Write( "]\n\t\t}" );

            // insert a comma for next library if we are not done yet
            if (numLibsOutput < m_LibraryStats.GetSize() - 1)
            {
                Write( ",\n\t\t" );
            }

            continue;
        }

        Write( "\n\t\t\t\t" );

        const uint32_t numObjects = ls->m_ObjectCount;

        // output
        const size_t numIncludes = incStats.GetSize();
        size_t numOutput = 0;
        for ( size_t i=0; i<numIncludes; ++i )
        {
            const IncludeStats & s = *incStats[ i ];
            const char * fileName = s.m_Node->GetName().Get();
            const uint32_t included = s.m_Count;
            const bool inPCH = s.m_InPCH;

            Write( "{" );
            Write( "\n\t\t\t\t\t" );

            Write( "\"Objects\": %u,\n\t\t\t\t\t", numObjects );
            Write( "\"Included\": %u,\n\t\t\t\t\t", included );
            Write( "\"PCH\": \"%s\",\n\t\t\t\t\t", inPCH ? "YES" : "no" );

            AStackString<> programName( fileName );
            JSON::Escape( programName );

            Write( "\"Name\": \"%s\"\n\t\t\t\t", programName.Get() );

            Write( "}" );

            if ( numOutput < numIncludes - 1 )
            {
                Write( ",\n\t\t\t\t" );
            }

            numOutput++;
        }

        Write( "\n\t\t\t] ");

        Write( "\n\t\t}" );

        if ( numLibsOutput < m_LibraryStats.GetSize() - 1 )
        {
            Write( ",\n\t\t" );
        }

        numLibsOutput++;
    }

    if ( numLibsOutput == 0 )
    {
        Write( "]" );
    }
    else 
    {
        Write( "\n\t]" );
    }
}
PRAGMA_DISABLE_POP_MSVC // warning C6262: Function uses '262212' bytes of stack

//------------------------------------------------------------------------------
