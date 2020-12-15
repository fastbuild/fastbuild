// TestLibrary.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"

// Core
#include <Core/Strings/AStackString.h>

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
    REGISTER_TEST( LibraryType )    // Test library detection code
REGISTER_TESTS_END

// LibraryType
//------------------------------------------------------------------------------
void TestLibrary::LibraryType() const
{
    #define TEST_LIBRARYTYPE( exeName, expectedFlag ) \
    do { \
        const uint32_t flags = LibraryNode::DetermineFlags( AStackString<>( "auto" ), \
                                                            AStackString<>( exeName ), \
                                                            AString::GetEmpty() ); \
        TEST_ASSERT( ( flags & expectedFlag ) == expectedFlag ); \
    } while( false )

    TEST_LIBRARYTYPE( "link",       LibraryNode::LIB_FLAG_LIB );
    TEST_LIBRARYTYPE( "lib",        LibraryNode::LIB_FLAG_LIB );
    TEST_LIBRARYTYPE( "ar",         LibraryNode::LIB_FLAG_AR );
    TEST_LIBRARYTYPE( "orbis-ar",   LibraryNode::LIB_FLAG_ORBIS_AR );
    TEST_LIBRARYTYPE( "\\ax",       LibraryNode::LIB_FLAG_GREENHILLS_AX );

    #undef TEST_LIBRARYTYPE
}

//------------------------------------------------------------------------------
