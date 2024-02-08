// TestWarnings.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Core/Strings/AStackString.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"

// TestWarnings
//------------------------------------------------------------------------------
class TestWarnings : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void WarningsAreShown() const;
    void ClangMacroExpansion() const;
    void PragmaMessageWarningsAreShown() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestWarnings )
    REGISTER_TEST( WarningsAreShown )
    #if defined( __WINDOWS__ )
        // TODDO: B Enable for other platforms (fix two-pass issues)
        REGISTER_TEST( ClangMacroExpansion )
        REGISTER_TEST( PragmaMessageWarningsAreShown )
    #endif
REGISTER_TESTS_END

// WarningsAreShown
//------------------------------------------------------------------------------
void TestWarnings::WarningsAreShown() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestWarnings/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "Warnings" ) );

    // Ensure warning was emitted
    #if defined( __WINDOWS__ )
        TEST_ASSERT( GetRecordedOutput().Find( "unreferenced local variable" ) );
    #else
        TEST_ASSERT( GetRecordedOutput().Find( "-Wunused-variable" ) );
    #endif
}

// PragmaMessageWarningsAreShown
//------------------------------------------------------------------------------
void TestWarnings::PragmaMessageWarningsAreShown() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestWarnings/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "PragmaMessage" ) );

    // Ensure message was emitted
    TEST_ASSERT( GetRecordedOutput().Find( "Optimization force disabled" ) );
}

// ClangMacroExpansion
//------------------------------------------------------------------------------
void TestWarnings::ClangMacroExpansion() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestWarnings/ClangMacroExpansion/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( "ClangMacroExpansion" ) );

    // Ensure message was emitted
    TEST_ASSERT( GetRecordedOutput().Find( "-Wtautological-unsigned-zero-compare" ) );
}

//------------------------------------------------------------------------------
