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
        REGISTER_TEST( PragmaMessageWarningsAreShown )
    #endif
    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        REGISTER_TEST( ClangMacroExpansion )
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
}

// ClangMacroExpansion
//------------------------------------------------------------------------------
void TestWarnings::ClangMacroExpansion() const
{
    #if defined( __WINDOWS__ )
        // TODO:A Check if this is still relevant for newer versions of Clang
        // The warning this test relies on has change and this test may need to
        // be updated
    #else
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestWarnings/ClangMacroExpansion/fbuild.bff";

        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.Build( "ClangMacroExpansion" ) );
    #endif
}

//------------------------------------------------------------------------------
