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

    void RemoveDirRecurse() const;
    void RemoveDirRecurseNoExist() const;
    void RemoveDirNoRecurse() const;
    void RemoveDirNoRemoveDirs() const;
    void RemoveDirNoRemoveRootDir() const;

    // Helpers
    void CreateFiles() const;

    static const char * const s_PathA;
    static const char * const s_FileA;
    static const char * const s_PathB;
    static const char * const s_FileB;
    static const char * const s_PathC;
    static const char * const s_FileC;
};

// Static Data
//------------------------------------------------------------------------------
/*static*/ const char * const TestRemoveDir::s_PathA = "../tmp/Test/RemoveDir/";
/*static*/ const char * const TestRemoveDir::s_FileA = "../tmp/Test/RemoveDir/File";
/*static*/ const char * const TestRemoveDir::s_PathB = "../tmp/Test/RemoveDir/SubDir/";
/*static*/ const char * const TestRemoveDir::s_FileB = "../tmp/Test/RemoveDir/SubDir/File";
/*static*/ const char * const TestRemoveDir::s_PathC = "../tmp/Test/RemoveDir/SubDir/SubDir/";
/*static*/ const char * const TestRemoveDir::s_FileC = "../tmp/Test/RemoveDir/SubDir/SubDir/File";

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestRemoveDir )
    REGISTER_TEST( RemoveDirRecurse )
    REGISTER_TEST( RemoveDirRecurseNoExist )
    REGISTER_TEST( RemoveDirNoRecurse )
    REGISTER_TEST( RemoveDirNoRemoveDirs )
    REGISTER_TEST( RemoveDirNoRemoveRootDir )
REGISTER_TESTS_END

// RemoveDirRecurse
//------------------------------------------------------------------------------
void TestRemoveDir::RemoveDirRecurse() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestRemoveDir/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    CreateFiles();

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "RemoveDirRecurse" ) );

    // Check files have been removed
    StackArray<AString> files;
    FileIO::GetFiles( AStackString( s_PathA ), AStackString( "*" ), true, &files );
    TEST_ASSERT( files.IsEmpty() );

    // Check empty dirs have been removed
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathA ) ) == false );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathB ) ) == false );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathC ) ) == false );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( 1, 1, Node::REMOVE_DIR_NODE );
    CheckStatsTotal( 2, 2 );
}

// RemoveDirRecurseNoExist
//------------------------------------------------------------------------------
void TestRemoveDir::RemoveDirRecurseNoExist() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestRemoveDir/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Ensure no files or directories exist
    EnsureFileDoesNotExist( s_FileA );
    EnsureFileDoesNotExist( s_FileB );
    EnsureFileDoesNotExist( s_FileC );
    EnsureDirDoesNotExist( s_PathC );
    EnsureDirDoesNotExist( s_PathB );
    EnsureDirDoesNotExist( s_PathA );

    // Build and ensure no failures when there is nothing to delete
    // (in particular the root dir not existing needs to be handled)
    TEST_ASSERT( fBuild.Build( "RemoveDirRecurse" ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( 1, 1, Node::REMOVE_DIR_NODE );
    CheckStatsTotal( 2, 2 );
}

// RemoveDirNoRecurse
//------------------------------------------------------------------------------
void TestRemoveDir::RemoveDirNoRecurse() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestRemoveDir/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    CreateFiles();

    // Build
    TEST_ASSERT( fBuild.Build( "RemoveDirNoRecurse" ) );

    // Check file in sub-directory was not removed
    StackArray<AString> files;
    FileIO::GetFiles( AStackString( s_PathA ), AStackString( "*" ), true, &files );
    TEST_ASSERT( files.GetSize() == 2 );
    files[ 0 ].Replace( '\\', '/' );
    files[ 1 ].Replace( '\\', '/' );
    TEST_ASSERT( files.Find( s_FileB ) );
    TEST_ASSERT( files.Find( s_FileC ) );

    // Dirs should not have been removed as files still exist
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathA ) ) );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathB ) ) );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathC ) ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( 1, 1, Node::REMOVE_DIR_NODE );
    CheckStatsTotal( 2, 2 );
}

// RemoveDirNoRemoveDirs
//------------------------------------------------------------------------------
void TestRemoveDir::RemoveDirNoRemoveDirs() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestRemoveDir/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    CreateFiles();

    // Build
    TEST_ASSERT( fBuild.Build( "RemoveDirNoRemoveDirs" ) );

    // Check files have been removed
    StackArray<AString> files;
    FileIO::GetFiles( AStackString( s_PathA ), AStackString( "*" ), true, &files );
    TEST_ASSERT( files.IsEmpty() );

    // Dirs should not have been removed
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathA ) ) );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathB ) ) );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathC ) ) );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( 1, 1, Node::REMOVE_DIR_NODE );
    CheckStatsTotal( 2, 2 );
}

// RemoveDirNoRemoveRootDir
//------------------------------------------------------------------------------
void TestRemoveDir::RemoveDirNoRemoveRootDir() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestRemoveDir/fbuild.bff";
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    CreateFiles();

    // Build
    TEST_ASSERT( fBuild.Build( "RemoveDirNoRemoveRootDir" ) );

    // Check files have been removed
    StackArray<AString> files;
    FileIO::GetFiles( AStackString( s_PathA ), AStackString( "*" ), true, &files );
    TEST_ASSERT( files.IsEmpty() );

    // Sub dirs should have been removed, but root dir should remain
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathA ) ) );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathB ) ) == false );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString( s_PathC ) ) == false );

    // Check stats: Seen, Built, Type
    CheckStatsNode( 1, 1, Node::DIRECTORY_LIST_NODE );
    CheckStatsNode( 1, 1, Node::REMOVE_DIR_NODE );
    CheckStatsTotal( 2, 2 );
}

//------------------------------------------------------------------------------
void TestRemoveDir::CreateFiles() const
{
    // Create folders and put files in them
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString( s_PathA ) ) );
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString( s_PathB ) ) );
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString( s_PathC ) ) );
    FileStream f;
    TEST_ASSERT( f.Open( s_FileA, FileStream::WRITE_ONLY ) );
    f.Close();
    TEST_ASSERT( f.Open( s_FileB, FileStream::WRITE_ONLY ) );
    f.Close();
    TEST_ASSERT( f.Open( s_FileC, FileStream::WRITE_ONLY ) );
    f.Close();

    // Check file are there
    Array<AString> files;
    FileIO::GetFiles( AStackString( s_PathA ), AStackString( "*" ), true, &files );
    TEST_ASSERT( files.GetSize() == 3 );
}

//------------------------------------------------------------------------------
