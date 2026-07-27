// TestFBuildDTLTO.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// TestFBuildDTLTO
//------------------------------------------------------------------------------
TEST_GROUP( TestFBuildDTLTO, FBuildTest )
{
};

//------------------------------------------------------------------------------
TEST_CASE( TestFBuildDTLTO, InitializeFromDTLTO )
{
    const char * testDir = "../tmp/Test/FBuildDTLTO/";
    const char * jsonFile = "../tmp/Test/FBuildDTLTO/dist-file.json";
    EnsureDirExists( testDir );
    MakeFile( jsonFile, R"({
        "common": {
            "linker_output": "app.exe",
            "args": [ "clang.exe" ],
            "inputs": []
        },
        "jobs": [
            {
                "args": [ "input.obj" ],
                "inputs": [ "input.obj" ],
                "outputs": [ "input.1.native.o" ]
            }
        ]
    })" );

    FBuildTestOptions options;
    options.m_DTLTOFile = jsonFile;
    options.m_SaveDBOnCompletion = true;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.GetNode( "all" ) );
    TEST_ASSERT( fBuild.GetOptions().m_SaveDBOnCompletion == false );
}

//------------------------------------------------------------------------------
TEST_CASE( TestFBuildDTLTO, MissingDTLTOFile )
{
    const char * jsonFile = "../tmp/Test/FBuildDTLTO/missing-dist-file.json";
    EnsureFileDoesNotExist( jsonFile );

    FBuildTestOptions options;
    options.m_DTLTOFile = jsonFile;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to open" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestFBuildDTLTO, MalformedDTLTOJson )
{
    const char * testDir = "../tmp/Test/FBuildDTLTO/";
    const char * jsonFile = "../tmp/Test/FBuildDTLTO/malformed-dist-file.json";
    EnsureDirExists( testDir );
    MakeFile( jsonFile, "not json" );

    FBuildTestOptions options;
    options.m_DTLTOFile = jsonFile;

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected '{'" ) );
}

