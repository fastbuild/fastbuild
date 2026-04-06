// TestAlias.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestAlias, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestAlias, MissingAliasTarget )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestAlias/alias.bff";
    options.m_ForceCleanBuild = true;

    // Parsing of BFF should be ok
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // building should fail
    TEST_ASSERT( fBuild.Build( "alias" ) == false );
}

//------------------------------------------------------------------------------
TEST_CASE( TestAlias, ReflectionAliasResolution_Case1 )
{
    // FAIL Case 1: An Alias to >1 item
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestAlias/Reflection/bad_string.bff";

        // Parsing of BFF should FAIL
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() == false );
        TEST_ASSERT( GetRecordedOutput().Find( "Error #1050" ) );
    }
}

// ReflectionAliasResolution_Case2
//------------------------------------------------------------------------------
TEST_CASE( TestAlias, ReflectionAliasResolution_Case2 )
{
    // FAIL Case 2: An Alias to >1 item (indirectly via another alias)
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestAlias/Reflection/bad_string_recurse.bff";

        // Parsing of BFF should FAIL
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() == false );
        TEST_ASSERT( GetRecordedOutput().Find( "Error #1050" ) );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAlias, ReflectionAliasResolution_Case3 )
{
    // OK Case 1: An alias to single item
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestAlias/Reflection/ok_string.bff";

        // Parsing of BFF should FAIL
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAlias, ReflectionAliasResolution_Case4 )
{
    // OK Case 2: An alias to single item (indirectly via another alias)
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestAlias/Reflection/ok_string_recurse.bff";

        // Parsing of BFF should FAIL
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestAlias, NonFileNodes )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestAlias/Reflection/ok_to_non_filenode.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
}

//------------------------------------------------------------------------------
