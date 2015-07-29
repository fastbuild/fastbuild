// TestObjectList.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Core/Strings/AStackString.h"

// TestObjectList
//------------------------------------------------------------------------------
class TestObjectList : public FBuildTest
{
private:
	DECLARE_TESTS

	// Tests
	void TestExcludedFiles() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestObjectList )
	REGISTER_TEST( TestExcludedFiles )		// Ensure files are correctly excluded
REGISTER_TESTS_END

// TestExcludedFiles
//------------------------------------------------------------------------------
void TestObjectList::TestExcludedFiles() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestObjectList/Exclusions/fbuild.bff";

	{
		FBuild fBuild( options );
		TEST_ASSERT( fBuild.Initialize() );

		TEST_ASSERT( fBuild.Build( AStackString<>( "ExcludeFileName" ) ) );
	}

	{
		FBuild fBuild( options );
		TEST_ASSERT( fBuild.Initialize() );

		TEST_ASSERT( fBuild.Build( AStackString<>( "ExcludeFilePath" ) ) );
	}

	{
		FBuild fBuild( options );
		TEST_ASSERT( fBuild.Initialize() );

		TEST_ASSERT( fBuild.Build( AStackString<>( "ExcludeFilePathRelative" ) ) );
	}
}

//------------------------------------------------------------------------------
