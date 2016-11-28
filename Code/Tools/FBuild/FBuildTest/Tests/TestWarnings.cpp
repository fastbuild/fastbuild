// TestWarnings.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Core/Strings/AStackString.h"

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
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestWarnings/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "Warnings" ) ) );
}

// PragmaMessageWarningsAreShown
//------------------------------------------------------------------------------
void TestWarnings::PragmaMessageWarningsAreShown() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestWarnings/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "PragmaMessage" ) ) );
}

// ClangMacroExpansion
//------------------------------------------------------------------------------
void TestWarnings::ClangMacroExpansion() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestWarnings/ClangMacroExpansion/fbuild.bff";

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    TEST_ASSERT( fBuild.Build( AStackString<>( "ClangMacroExpansion" ) ) );
}

//------------------------------------------------------------------------------
