// TestLibrary.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"

// TestLibrary
//------------------------------------------------------------------------------
class TestLibrary : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void LibraryType() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestLibrary )
    REGISTER_TEST(LibraryType)                 // Test library detection code
REGISTER_TESTS_END

// LinkerType
//------------------------------------------------------------------------------
void TestLibrary::LibraryType() const
{
    uint32_t flags = 0;

    flags = LibraryNode::DetermineFlags( AString( "auto" ), AString( "link" ), AString( "" ));
    TEST_ASSERT(( flags & LibraryNode::LIB_FLAG_LIB ) == LibraryNode::LIB_FLAG_LIB );

    flags = LibraryNode::DetermineFlags( AString( "auto" ), AString( "lib" ), AString( "" ));
    TEST_ASSERT(( flags & LibraryNode::LIB_FLAG_LIB ) == LibraryNode::LIB_FLAG_LIB );

    flags = LibraryNode::DetermineFlags( AString( "auto" ), AString( "ar" ), AString( "" ));
    TEST_ASSERT(( flags & LibraryNode::LIB_FLAG_AR ) == LibraryNode::LIB_FLAG_AR );

    flags = LibraryNode::DetermineFlags( AString( "auto" ), AString( "orbis-ar" ), AString( "" ));
    TEST_ASSERT(( flags & LibraryNode::LIB_FLAG_ORBIS_AR ) == LibraryNode::LIB_FLAG_ORBIS_AR );

    flags = LibraryNode::DetermineFlags( AString( "auto" ), AString( "\\ax" ), AString( "" ));
    TEST_ASSERT(( flags & LibraryNode::LIB_FLAG_GREENHILLS_AX ) == LibraryNode::LIB_FLAG_GREENHILLS_AX );
}

//------------------------------------------------------------------------------
