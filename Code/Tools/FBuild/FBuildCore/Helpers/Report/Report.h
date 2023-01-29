// Report - An interface for report generation
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Containers/Array.h"
#include "Core/Env/MSVCStaticAnalysis.h"
#include "Core/Mem/MemPoolBlock.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
struct FBuildStats;
class Dependencies;
class Node;
class NodeGraph;

// Report
//------------------------------------------------------------------------------
class Report
{
public:
    static void Generate( const AString & reportType,
                          const NodeGraph & nodeGraph,
                          const FBuildStats & stats );

protected:
    Report();
    virtual ~Report();

    virtual void Generate( const NodeGraph & nodeGraph, const FBuildStats & stats ) = 0;
    virtual void Save() const = 0;

    enum : uint32_t
    {
        eNodeNotSeen    = 0,
        eNodeSeen       = 1,
    };

    class LibraryStats
    {
    public:
        const Node *    m_Library;
        uint32_t        m_CPUTimeMS;
        uint32_t        m_ObjectCount;
        uint32_t        m_ObjectCount_OutOfDate;
        uint32_t        m_ObjectCount_Cacheable;
        uint32_t        m_ObjectCount_CacheHits;
        uint32_t        m_ObjectCount_CacheStores;
        uint32_t        m_CacheTimeMS;

        bool operator < ( const LibraryStats & other ) const { return m_CPUTimeMS > other.m_CPUTimeMS; }
    };

    class IncludeStats
    {
    public:
        const Node *    m_Node;
        uint32_t        m_Count;
        bool            m_InPCH;

        bool operator < ( const IncludeStats & other ) const { return m_Count > other.m_Count; }

        IncludeStats *  m_Next; // in-place hash map chain
    };

    class IncludeStatsMap
    {
    public:
        IncludeStatsMap();
        ~IncludeStatsMap();

        IncludeStats * Find( const Node * node ) const;
        IncludeStats * Insert( const Node * node );

        void Flatten( Array< const IncludeStats * > & stats ) const;
    protected:
        IncludeStats * m_Table[ 65536 ];
        MemPoolBlock m_Pool;
    };

    // Helpers to format text
    void Write( MSVC_SAL_PRINTF const char * fmtString, ... ) FORMAT_STRING( 2, 3 );
    void GetReportDateTime( AString & outReportDateTime ) const;
    const char * GetTimeTakenPlaceholder() const { return "<TIME_TAKEN_PLACEHOLDER>"; }
    void FixupTimeTakenPlaceholder();

    // gather stats
    void GetLibraryStats( const NodeGraph & nodeGraph, const FBuildStats & stats );
    void GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Node * node, LibraryStats * currentLib ) const;
    void GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Dependencies & dependencies, LibraryStats * currentLib ) const;
    void GetIncludeFilesRecurse( IncludeStatsMap & incStats, const Node * node ) const;
    void AddInclude( IncludeStatsMap & incStats, const Node * node, const Node * parentNode ) const;

    // intermediate collected data
    Array< LibraryStats * > m_LibraryStats;
    Timer m_Timer;

    // final output
    AString m_Output;
};

//------------------------------------------------------------------------------
