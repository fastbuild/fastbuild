// TestDTLTOJsonParser.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/Helpers/DTLTOJsonParser.h"

// TestDTLTOJsonParser
//------------------------------------------------------------------------------
TEST_GROUP( TestDTLTOJsonParser, FBuildTest )
{
};

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, MatchObject )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/common.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() );
    TEST_ASSERT( parser.GetData().m_LinkerOutput == "out.exe" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, LoadMissingFile )
{
    DTLTOJsonParser parser( "does-not-exist.json" );
    TEST_ASSERT( parser.Load() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to open" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorExpectedRootObject )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/bad-root-type.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected '{'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorExpectedPropertyName )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/empty-object.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected property name" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorExpectedColonAfterProperty )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/missing-colon.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected ':' after property 'common'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorUnknownProperty )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/unknown-property.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: unknown property 'unknown'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorExpectedCommaAfterPropertyValue )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/missing-comma.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected ',' after value for property 'common'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorFailedToParseCommonValue )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/common-wrong-type.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to parse property value for 'common'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorExpectedStringForLinkerOutput )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/linker-output-wrong-type.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: expected string for 'linker_output'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorFailedToParseLinkerOutputValue )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/linker-output-wrong-type.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: failed to parse property value for 'linker_output'" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorUnterminatedString )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/unterminated-string.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: unterminated string" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOJsonParser, ErrorUnknownEscape )
{
    DTLTOJsonParser parser( "Tools/FBuild/FBuildTest/Data/TestDTLTOJsonParser/unknown-escape.json" );
    TEST_ASSERT( parser.Load() );
    TEST_ASSERT( parser.Parse() == false );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: unknown escape '\\q'" ) );
}

//------------------------------------------------------------------------------
