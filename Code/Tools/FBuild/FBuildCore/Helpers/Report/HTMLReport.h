// HTMLReport
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Helpers/Report/Report.h"

// Forward Declarations
//------------------------------------------------------------------------------
struct FBuildStats;

// HTMLReport
//------------------------------------------------------------------------------
class HTMLReport : public Report
{
public:
    HTMLReport();
    virtual ~HTMLReport() override;

    virtual void Generate(const NodeGraph & nodeGraph, const FBuildStats & stats) override;
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

    class PieItem
    {
    public:
        PieItem( const char * label, float value, uint32_t color, void * userData = nullptr )
            : m_Label( label )
            , m_Value( value )
            , m_Color( color )
            , m_UserData( userData )
        {
        }

        const char *    m_Label;
        float           m_Value;
        uint32_t        m_Color;
        void *          m_UserData;

        bool operator < ( const PieItem & other ) const { return m_Value > other.m_Value; }
    };
    
    enum { DEFAULT_TABLE_WIDTH = 990 };

    // Helpers
    void DoTableStart( uint32_t width = DEFAULT_TABLE_WIDTH, const char * id = nullptr, bool hidden = false );
    void DoTableStop();
    void DoToggleSection( size_t numMore = 0 );
    void DoSectionTitle( const char * sectionName, const char * sectionId );
    void DoPieChart( const Array< PieItem > & items, const char * units );

    // intermediate collected data
    uint32_t m_NumPieCharts = 0;
};

//------------------------------------------------------------------------------
