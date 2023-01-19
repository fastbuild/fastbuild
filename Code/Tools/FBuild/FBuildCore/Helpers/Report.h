// Report - An interface for report generation
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
    Report( size_t initialCapacity, bool resizeable );
    virtual ~Report();

    virtual void Generate( const FBuildStats& stats ) = 0;
    virtual void Save() const = 0;
    
protected:
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
        const Node*     node;
        uint32_t        count;
        bool            inPCH;

        bool operator < ( const IncludeStats& other ) const { return count > other.count; }

        IncludeStats* m_Next; // in-place hash map chain
    };

    class IncludeStatsMap
    {
    public:
        IncludeStatsMap();
        ~IncludeStatsMap();

        IncludeStats* Find( const Node* node ) const;
        IncludeStats* Insert( const Node* node );

        void Flatten( Array< const IncludeStats* >& stats ) const;
    protected:
        IncludeStats* m_Table[65536];
        MemPoolBlock m_Pool;
    };

    // Helper to format some text
    void Write(MSVC_SAL_PRINTF const char* fmtString, ...) FORMAT_STRING(2, 3);

    // gather stats
    void GetLibraryStats( const FBuildStats & stats );
    void GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Node * node, LibraryStats * currentLib ) const;
    void GetLibraryStatsRecurse( Array< LibraryStats * > & libStats, const Dependencies & dependencies, LibraryStats * currentLib ) const;
    void GetIncludeFilesRecurse( IncludeStatsMap& incStats, const Node* node ) const;
    void AddInclude( IncludeStatsMap& incStats, const Node* node, const Node* parentNode ) const;

    // intermediate collected data
    Array< LibraryStats * > m_LibraryStats;

    // final output
    AString m_Output;
};

//------------------------------------------------------------------------------
