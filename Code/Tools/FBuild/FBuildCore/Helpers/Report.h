// Report - Build Report Generator
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/MSVCStaticAnalysis.h"
#include "Core/Mem/MemPoolBlock.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
struct FBuildStats;
class Dependencies;
class Node;

// Report
//------------------------------------------------------------------------------
class Report
{
public:
    Report();
    ~Report();

    void Generate( const FBuildStats & stats );
    void Save() const;

private:
    // Report sections
    void CreateHeader();
    void CreateTitle();
    void CreateOverview( const FBuildStats & stats );
    void DoCacheStats( const FBuildStats & stats );
    void DoCPUTimeByType( const FBuildStats & stats );
    void DoCPUTimeByItem( const FBuildStats & stats );
    void DoCPUTimeByLibrary();
    void DoIncludes();

    void CreateFooter();

    struct PieItem
    {
        PieItem( const char * l, float v, uint32_t c, void * u = nullptr )
            : label( l )
            , value( v )
            , color( c )
            , userData( u )
        {
        }

        const char *    label;
        float           value;
        uint32_t        color;
        void *          userData;

        bool operator < ( const PieItem & other ) const { return value > other.value; }
    };

    struct LibraryStats
    {
        const Node *    library;
        uint32_t        cpuTimeMS;
        uint32_t        objectCount;
        uint32_t        objectCount_OutOfDate;
        uint32_t        objectCount_Cacheable;
        uint32_t        objectCount_CacheHits;
        uint32_t        objectCount_CacheStores;
        uint32_t        cacheTimeMS;

        bool operator < ( const LibraryStats & other ) const { return cpuTimeMS > other.cpuTimeMS; }
    };

    struct IncludeStats
    {
        const Node *    node;
        uint32_t        count;
        bool            inPCH;

        bool operator < ( const IncludeStats & other ) const { return count > other.count; }

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

    enum { DEFAULT_TABLE_WIDTH = 990 };

    // Helpers
    void DoTableStart( uint32_t width = DEFAULT_TABLE_WIDTH, const char * id = nullptr, bool hidden = false );
    void DoTableStop();
    void DoToggleSection( size_t numMore = 0 );
    void DoSectionTitle( const char * sectionName, const char * sectionId );
    void DoPieChart( const Array< PieItem > & items, const char * units );

    // Helper to format some text
    void Write( MSVC_SAL_PRINTF const char * fmtString, ... ) FORMAT_STRING( 2, 3 );

    // gather stats
    void GetLibraryStats( const FBuildStats & stats );
    void GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Node * node, LibraryStats * currentLib ) const;
    void GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Dependencies & dependencies, LibraryStats * currentLib ) const;
    void GetIncludeFilesRecurse( IncludeStatsMap & incStats, const Node * node) const;
    void AddInclude( IncludeStatsMap & incStats, const Node * node, const Node * parentNode) const;

    // intermediate collected data
    Array< LibraryStats * > m_LibraryStats;
    uint32_t m_NumPieCharts;

    // final output
    AString m_Output;
};

//------------------------------------------------------------------------------
