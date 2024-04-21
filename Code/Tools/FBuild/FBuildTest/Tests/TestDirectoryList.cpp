// TestDirectoryList.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// TestGraph
//------------------------------------------------------------------------------
class TestDirectoryList : public FBuildTest
{
private:
    DECLARE_TESTS

    void Build() const;
    void Names() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestDirectoryList )
    REGISTER_TEST( Build )
    REGISTER_TEST( Names )
REGISTER_TESTS_END

// Build
//------------------------------------------------------------------------------
void TestDirectoryList::Build() const
{
    FBuild fb;
    NodeGraph ng;

    // Generate a valid DirectoryListNode name
    AStackString<> name;
    #if defined( __WINDOWS__ )
        const AStackString<> testFolder( "Tools\\FBuild\\FBuildTest\\Data\\TestGraph\\" );
    #else
        const AStackString<> testFolder( "Tools/FBuild/FBuildTest/Data/TestGraph/" );
    #endif
    Array< AString > patterns;
    patterns.EmplaceBack( "library.*" );
    DirectoryListNode::FormatName( testFolder,
                                   &patterns,
                                   true, // recursive
                                   false, // Don't include read-only status in hash
                                   false, // Don't include directories
                                   Array< AString >(), // excludePaths,
                                   Array< AString >(), // excludeFiles,
                                   Array< AString >(), // excludePatterns,
                                   name );

    // create the node, and make sure we can access it by name
    DirectoryListNode * node = ng.CreateNode<DirectoryListNode>( name );
    node->m_Path = testFolder;
    node->m_Patterns = patterns;
    const BFFToken * token = nullptr;
    TEST_ASSERT( node->Initialize( ng, token, nullptr ) );
    TEST_ASSERT( ng.FindNode( name ) == node );

    TEST_ASSERT( fb.Build( node ) );

    // make sure we got the expected results
    TEST_ASSERT( node->GetFiles().GetSize() == 2 );
    #if defined( __WINDOWS__ )
        const char * fileName1 = "Tools\\FBuild\\FBuildTest\\Data\\TestGraph\\library.cpp";
        const char * fileName2 = "Tools\\FBuild\\FBuildTest\\Data\\TestGraph\\library.o";
    #else
        const char * fileName1 = "Data/TestGraph/library.cpp";
        const char * fileName2 = "Data/TestGraph/library.o";
    #endif

    // returned order depends on file system
    if ( node->GetFiles()[ 0 ].m_Name.EndsWith( fileName1 ) )
    {
        TEST_ASSERT( node->GetFiles()[ 1 ].m_Name.EndsWith( fileName2 ) );
    }
    else
    {
        TEST_ASSERT( node->GetFiles()[ 0 ].m_Name.EndsWith( fileName2 ) );
        TEST_ASSERT( node->GetFiles()[ 1 ].m_Name.EndsWith( fileName1 ) );
    }
}

// Names
//------------------------------------------------------------------------------
void TestDirectoryList::Names() const
{
    // Generate various directory list node names and ensure they are unique
    // given differing search params

    // Use the same path in each case
    AStackString<> path( "Tools/FBuild/FBuildTest/Data/TestDirectoryList/" );
    PathUtils::FixupFolderPath( path );

    // Build a list of node names we can verify for uniqueness later
    StackArray<AString> names;

    // All files with no special options
    DirectoryListNode::FormatName( path,
                                   nullptr,             // patterns (.*)
                                   false,               // not recursive
                                   false,               // don't include read only status 
                                   false,               // don't include dirs
                                   Array<AString>(),    // exclude paths
                                   Array<AString>(),    // exclude files
                                   Array<AString>(),    // exclude patterns
                                   names.EmplaceBack() ); // result

    // Patterns
    {
        StackArray<AString> patterns;
        patterns.EmplaceBack( ".cpp" );
        DirectoryListNode::FormatName( path,
                                       &patterns, // patterns
                                       false,
                                       false,
                                       false,
                                       Array<AString>(),
                                       Array<AString>(),
                                       Array<AString>(),
                                       names.EmplaceBack() );
    }

    // Recursive
    DirectoryListNode::FormatName( path,
                                   nullptr,
                                   true, // recursive
                                   false,
                                   false,
                                   Array<AString>(),
                                   Array<AString>(),
                                   Array<AString>(),
                                   names.EmplaceBack() );

    // Include RO status
    DirectoryListNode::FormatName( path,
                                   nullptr,
                                   false,
                                   true, // include read only status
                                   false,
                                   Array<AString>(),
                                   Array<AString>(),
                                   Array<AString>(),
                                   names.EmplaceBack() );

    // Include dirs
    DirectoryListNode::FormatName( path,
                                   nullptr,
                                   false,
                                   false,
                                   true, // include dirs
                                   Array<AString>(),
                                   Array<AString>(),
                                   Array<AString>(),
                                   names.EmplaceBack() );

    // Exclude Paths
    {
        StackArray<AString> excludePaths;
        excludePaths.EmplaceBack( "path/path/" );
        PathUtils::FixupFolderPath( excludePaths[ 0 ] );
        DirectoryListNode::FormatName( path,
                                       nullptr,
                                       false,
                                       false,
                                       false,
                                       excludePaths, // exclude paths
                                       Array<AString>(),
                                       Array<AString>(),
                                       names.EmplaceBack() );
    }

    // Exclude Files
    {
        StackArray<AString> excludeFiles;
        excludeFiles.EmplaceBack( "file.cpp" );
        DirectoryListNode::FormatName( path,
                                       nullptr,
                                       false,
                                       false,
                                       false,
                                       Array<AString>(),
                                       excludeFiles, // exclude files
                                       Array<AString>(),
                                       names.EmplaceBack() );
    }

    // Exclude Patterns
    {
        StackArray<AString> excludePatterns;
        excludePatterns.EmplaceBack( "*.cpp" );
        DirectoryListNode::FormatName( path,
                                       nullptr,
                                       false,
                                       false,
                                       false,
                                       Array<AString>(),
                                       Array<AString>(),
                                       excludePatterns, // exclude patterns
                                       names.EmplaceBack() );
    }

    // Ensure all generated names were unique
    names.Sort();
    for ( size_t i = 1; i < names.GetSize(); ++i )
    {
        TEST_ASSERT( names[ i ] != names[ i - 1 ] );
    }
}

//------------------------------------------------------------------------------
