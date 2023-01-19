// HtmlReport - Build HtmlReport Generator
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Helpers/Report.h"


// Forward Declarations
//------------------------------------------------------------------------------
struct FBuildStats;
class Dependencies;
class Node;

// HtmlReport
//------------------------------------------------------------------------------
class HtmlReport : public Report
{
public:
    HtmlReport();
    ~HtmlReport();

    virtual void Generate( const FBuildStats & stats ) override;
    virtual void Save() const override;

private:
    // HtmlReport sections
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
    
    enum { DEFAULT_TABLE_WIDTH = 990 };

    // Helpers
    void DoTableStart( uint32_t width = DEFAULT_TABLE_WIDTH, const char * id = nullptr, bool hidden = false );
    void DoTableStop();
    void DoToggleSection( size_t numMore = 0 );
    void DoSectionTitle( const char * sectionName, const char * sectionId );
    void DoPieChart( const Array< PieItem > & items, const char * units );

    // intermediate collected data
    uint32_t m_NumPieCharts;

};

//------------------------------------------------------------------------------
