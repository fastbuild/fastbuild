// TestRemoveDir.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/RemoveDirNode.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// TestRemoveDir
//------------------------------------------------------------------------------
class TestRemoveDir : public FBuildTest
{
private:
    DECLARE_TESTS

    void CreateNode() const;
    void RemoveDir() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestRemoveDir )
    REGISTER_TEST( CreateNode )
    REGISTER_TEST( RemoveDir )
REGISTER_TESTS_END

// CreateNode
//------------------------------------------------------------------------------
void TestRemoveDir::CreateNode() const
{
    FBuild fb;
    NodeGraph ng;

    RemoveDirNode * removeDirNode = ng.CreateRemoveDirNode( AStackString<>( "name" ) );

    TEST_ASSERT( removeDirNode->GetType() == Node::REMOVE_DIR_NODE );
    TEST_ASSERT( RemoveDirNode::GetTypeS() == Node::REMOVE_DIR_NODE );
    TEST_ASSERT( AStackString<>( "RemoveDir" ) == removeDirNode->GetTypeName() );
}

// RemoveDir
//------------------------------------------------------------------------------
void TestRemoveDir::RemoveDir() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestRemoveDir/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Create folders and put files in them
    const char * pathA = "../tmp/Test/RemoveDir/";
    const char * fileA = "../tmp/Test/RemoveDir/File";
    const char * pathB = "../tmp/Test/RemoveDir/SubDir/";
    const char * fileB = "../tmp/Test/RemoveDir/SubDir/File";
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString<>( pathA ) ) );
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString<>( pathB ) ) );
    FileStream f;
    TEST_ASSERT( f.Open( fileA, FileStream::WRITE_ONLY ) );
    f.Close();
    TEST_ASSERT( f.Open( fileB, FileStream::WRITE_ONLY ) );
    f.Close();

    // Check file are there
    Array< AString > files;
    FileIO::GetFiles( AStackString<>( pathA ), AStackString<>( "*" ), true, &files );
    TEST_ASSERT( files.GetSize() == 2 );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "RemoveDir" ) );

    // Check files are NOT there
    files.Clear();
    FileIO::GetFiles( AStackString<>( pathA ), AStackString<>( "*" ), true, &files );
    TEST_ASSERT( files.GetSize() == 0 );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::DIRECTORY_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::REMOVE_DIR_NODE );
    CheckStatsTotal( 2,     2 );
}

//------------------------------------------------------------------------------
