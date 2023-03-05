// JSONReport
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Helpers/Report/Report.h"

// Forward Declarations
//------------------------------------------------------------------------------
struct FBuildStats;

// JSONReport
//------------------------------------------------------------------------------
class JSONReport : public Report
{
public:
    JSONReport();
    virtual ~JSONReport() override;

    virtual void Generate( const NodeGraph & nodeGraph, const FBuildStats & stats ) override;
    virtual void Save() const override;

private:
    // JsonReport sections
    void CreateOverview( const FBuildStats & stats );
    void DoCacheStats( const FBuildStats & stats );
    void DoCPUTimeByType( const FBuildStats & stats );
    void DoCPUTimeByItem( const FBuildStats & stats );
    void DoCPUTimeByLibrary();
    void DoIncludes();

    class TimingStats
    {
    public:
        TimingStats( const char * label, float value, void * userData = nullptr )
            : m_Label( label )
            , m_Value( value )
            , m_UserData( userData )
        {
        }

        const char *    m_Label;
        float           m_Value;
        void *          m_UserData;

        bool operator < ( const TimingStats& other ) const { return m_Value > other.m_Value; }
    };
};

//------------------------------------------------------------------------------
