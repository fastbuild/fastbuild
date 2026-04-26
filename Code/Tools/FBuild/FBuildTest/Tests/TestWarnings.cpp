// TestWarnings.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Core/Strings/AStackString.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestWarnings, FBuildTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestWarnings, WarningsAreShown )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestWarnings/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "Warnings" ) );

    // Ensure warning was emitted
#if defined( __WINDOWS__ )
    TEST_ASSERT( GetRecordedOutput().Find( "unreferenced local variable" ) );
#else
    TEST_ASSERT( GetRecordedOutput().Find( "-Wunused-variable" ) );
#endif
}

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ ) // TODO:B Enable for other platforms (fix two-pass issues)
TEST_CASE( TestWarnings, PragmaMessageWarningsAreShown )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestWarnings/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "PragmaMessage" ) );

    // Ensure message was emitted
    TEST_ASSERT( GetRecordedOutput().Find( "Optimization force disabled" ) );
}
#endif

//------------------------------------------------------------------------------
#if defined( __WINDOWS__ ) // TODO:B Enable for other platforms (fix two-pass issues)
TEST_CASE( TestWarnings, ClangMacroExpansion )
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestWarnings/ClangMacroExpansion/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "ClangMacroExpansion" ) );

    // Ensure message was emitted
    TEST_ASSERT( GetRecordedOutput().Find( "-Wtautological-unsigned-zero-compare" ) );
}
#endif

//------------------------------------------------------------------------------
