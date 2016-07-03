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
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestWarnings )
	REGISTER_TEST( WarningsAreShown )
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

//------------------------------------------------------------------------------
