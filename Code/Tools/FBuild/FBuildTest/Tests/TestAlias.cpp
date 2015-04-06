// TestAlias.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"

#include "Core/Strings/AStackString.h"

// TestAlias
//------------------------------------------------------------------------------
class TestAlias : public FBuildTest
{
private:
	DECLARE_TESTS

	// Tests
	void MissingAliasTarget() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestAlias )
	REGISTER_TEST( MissingAliasTarget )
REGISTER_TESTS_END

// MissingAliasTarget
//------------------------------------------------------------------------------
void TestAlias::MissingAliasTarget() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestAlias/alias.bff";
	options.m_ForceCleanBuild = true;

	// Parsing of BFF should be ok
	FBuild fBuild( options );
	TEST_ASSERT( fBuild.Initialize() );

	// building should fail
	TEST_ASSERT( fBuild.Build( AStackString<>( "alias" ) ) == false );
}

//------------------------------------------------------------------------------
