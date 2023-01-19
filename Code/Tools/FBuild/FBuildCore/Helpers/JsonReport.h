// JsonReport - Build JsonReport Generator
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

// JsonReport
//------------------------------------------------------------------------------
class JsonReport : public Report
{
public:
    JsonReport();
    ~JsonReport();

    virtual void Generate( const FBuildStats & stats ) override;
    virtual void Save() const override;

private:
    // JsonReport sections
    void CreateOverview( const FBuildStats & stats );
    void DoCacheStats( const FBuildStats & stats );
    void DoCPUTimeByType( const FBuildStats & stats );
    void DoCPUTimeByItem( const FBuildStats & stats );
    void DoCPUTimeByLibrary();
    void DoIncludes();

    struct TimingStats
    {
        TimingStats( const char * l, float v, void * u = nullptr )
            : label( l )
            , value( v )
            , userData( u )
        {
        }

        const char *    label;
        float           value;
        void *          userData;

        bool operator < ( const TimingStats& other ) const { return value > other.value; }
    };
};

//------------------------------------------------------------------------------
