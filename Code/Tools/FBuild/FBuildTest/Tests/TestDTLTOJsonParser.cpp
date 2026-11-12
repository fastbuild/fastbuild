// TestDTLTOJsonParser.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/Helpers/DTLTOJsonParser.h"

#include "Core/Strings/AStackString.h"

// TestDTLTOJsonParser
//------------------------------------------------------------------------------
TEST_GROUP( TestDTLTOJsonParser, FBuildTest )
{
};

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Object_ErrorExpectedOpeningBrace )
{
    AStackString<> json( R"([])" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected '{'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Object_ErrorExpectedPropertyName )
{
    AStackString<> json( R"({})" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected property name" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Object_ErrorExpectedColonAfterProperty )
{
    AStackString<> json( R"({ "common" })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected ':' after property 'common'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Object_ErrorUnknownProperty )
{
    AStackString<> json( R"({ "unknown": 1 })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: unknown property 'unknown'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Object_ErrorExpectedCommaAfterPropertyValue )
{
    AStackString<> json( R"({ "common": { "linker_output": "out.exe" } "jobs": [] })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected ',' after value for property 'common'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, String_ErrorExpectedString )
{
    AStackString<> json( R"({ "common": { "linker_output": 1 } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected string" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, String_ErrorUnterminatedString )
{
    AStackString<> json( R"({ "common": { "linker_output": "out.exe }})" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: unterminated string" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, String_ErrorUnknownEscape )
{
    AStackString<> json( R"({ "common": { "linker_output": "out\q.exe" } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: unknown escape '\\q'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Array_MatchEmptyArray )
{
    AStackString<> json( R"({ "common": { "args": [] } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() );
    TEST_ASSERT( parser.GetData().m_CommonArgs.GetSize() == 0 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Array_ErrorExpectedArrayBracket )
{
    AStackString<> json( R"({ "common": { "args": "not-an-array" } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected '['" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, Array_ErrorExpectedCommaOrBracketInArray )
{
    AStackString<> json( R"({ "common": { "args": [ "-O2" "-flto" ] } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected ',' or ']' in array" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorFailedToReadCommon )
{
    AStackString<> json( R"({ "common": 1 })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to read property 'common'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, MatchLinkerOutput )
{
    AStackString<> json( R"({ "common": { "linker_output": "out.exe" } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() );
    TEST_ASSERT( parser.GetData().m_LinkerOutput == "out.exe" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorFailedToReadLinkerOutput )
{
    AStackString<> json( R"({ "common": { "linker_output": 1 } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to read property 'linker_output'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, MatchArgs )
{
    AStackString<> json( R"({ "common": { "args": [ "-O2", "-flto" ] } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() );

    const Array<AString> & args = parser.GetData().m_CommonArgs;
    TEST_ASSERT( args.GetSize() == 2 );
    TEST_ASSERT( args[ 0 ] == "-O2" );
    TEST_ASSERT( args[ 1 ] == "-flto" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorFailedToReadArgs )
{
    AStackString<> json( R"({ "common": { "args": [ "-O2", 1 ] } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to read property 'args'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, MatchInputs )
{
    AStackString<> json( R"({ "common": { "inputs": [ "app_main.c.obj", "math.c.obj", "path with spaces.obj" ] } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() );

    const Array<AString> & inputs = parser.GetData().m_CommonInputs;
    TEST_ASSERT( inputs.GetSize() == 3 );
    TEST_ASSERT( inputs[ 0 ] == "app_main.c.obj" );
    TEST_ASSERT( inputs[ 1 ] == "math.c.obj" );
    TEST_ASSERT( inputs[ 2 ] == "path with spaces.obj" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorFailedToReadInputs )
{
    AStackString<> json( R"({ "common": { "inputs": [ "a.obj", 1 ] } })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to read property 'inputs'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, MatchJobs )
{
    AStackString<> json( R"({
        "jobs": [
            { "args": [ "a.obj", "-o", "a.o" ], "inputs": [ "a.obj", "a.thinlto.bc" ], "outputs": [ "a.o" ] },
            { "args": [], "inputs": [ "b.obj" ], "outputs": [ "b.o" ] }
        ]
    })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() );

    const Array<DTLTOData::Job> & jobs = parser.GetData().m_Jobs;
    TEST_ASSERT( jobs.GetSize() == 2 );

    TEST_ASSERT( jobs[ 0 ].m_Args.GetSize() == 3 );
    TEST_ASSERT( jobs[ 0 ].m_Args[ 0 ] == "a.obj" );
    TEST_ASSERT( jobs[ 0 ].m_Args[ 2 ] == "a.o" );
    TEST_ASSERT( jobs[ 0 ].m_Inputs.GetSize() == 2 );
    TEST_ASSERT( jobs[ 0 ].m_Inputs[ 1 ] == "a.thinlto.bc" );
    TEST_ASSERT( jobs[ 0 ].m_Outputs.GetSize() == 1 );
    TEST_ASSERT( jobs[ 0 ].m_Outputs[ 0 ] == "a.o" );

    TEST_ASSERT( jobs[ 1 ].m_Args.GetSize() == 0 );
    TEST_ASSERT( jobs[ 1 ].m_Inputs.GetSize() == 1 );
    TEST_ASSERT( jobs[ 1 ].m_Outputs[ 0 ] == "b.o" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorFailedToReadJobs )
{
    AStackString<> json( R"({ "jobs": [ { "outputs": [ 1 ] } ] })" );
    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to read property 'jobs'" ) );
}

//------------------------------------------------------------------------------
