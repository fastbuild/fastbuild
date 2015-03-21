// TestPathUtils.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// TestPathUtils
//------------------------------------------------------------------------------
class TestPathUtils : public UnitTest
{
private:
	DECLARE_TESTS

	// Increment
	void TestFixupFolderPath() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestPathUtils )
	// Increment
	REGISTER_TEST( TestFixupFolderPath )
REGISTER_TESTS_END

// TestFixupFolderPath
//------------------------------------------------------------------------------
void TestPathUtils::TestFixupFolderPath() const
{
	#define DOCHECK( before, after ) \
	{ \
		AStackString<> path( before ); \
		PathUtils::FixupFolderPath( path ); \
		TEST_ASSERT( path == after ); \
	}

	#if defined( __WINDOWS__ )
		// standard windows path
		DOCHECK( "c:\\folder", "c:\\folder\\" )

		// redundant slashes
		DOCHECK( "c:\\folder\\\\thing", "c:\\folder\\thing\\" )

		// UNC path double slash is preserved
		DOCHECK( "\\\\server\\folder", "\\\\server\\folder\\" )
	#endif

	#undef DOCHECK
}

//------------------------------------------------------------------------------
