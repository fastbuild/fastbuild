// TestGraph.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CSNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/RemoveDirNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/Conversions.h"
#include "Core/Math/xxHash.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"

// TestGraph
//------------------------------------------------------------------------------
class TestGraph : public FBuildTest
{
private:
    DECLARE_TESTS

    void EmptyGraph() const;
    void TestNodeTypes() const;
    void TestCleanPath() const;
    void TestCleanPathPartial() const;
    void SingleFileNode() const;
    void SingleFileNodeMissing() const;
    void TestSerialization() const;
    void TestDeepGraph() const;
    void TestNoStopOnFirstError() const;
    void DBLocationChanged() const;
    void DBCorrupt() const;
    void BFFDirtied() const;
    void DBVersionChanged() const;
    void FixupErrorPaths() const;
    void CyclicDependency() const;
    void DBLocation() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestGraph )
    REGISTER_TEST( EmptyGraph )
    REGISTER_TEST( TestNodeTypes )
    REGISTER_TEST( TestCleanPath )
    REGISTER_TEST( TestCleanPathPartial )
    REGISTER_TEST( SingleFileNode )
    REGISTER_TEST( SingleFileNodeMissing )
    REGISTER_TEST( TestSerialization )
    REGISTER_TEST( TestDeepGraph )
    REGISTER_TEST( TestNoStopOnFirstError )
    REGISTER_TEST( DBLocationChanged )
    REGISTER_TEST( DBCorrupt )
    REGISTER_TEST( BFFDirtied )
    REGISTER_TEST( DBVersionChanged )
    REGISTER_TEST( FixupErrorPaths )
    REGISTER_TEST( CyclicDependency )
    REGISTER_TEST( DBLocation )
REGISTER_TESTS_END

// NodeTestHelper
//------------------------------------------------------------------------------
// Fake node to allow access to private internals
class NodeTestHelper : public Node
{
    REFLECT_DECLARE( NodeTestHelper )
public:
    NodeTestHelper()
        : Node( Node::PROXY_NODE )
    {
        SetName( AStackString( "placeholder" ) );
    }
    virtual bool Initialize( NodeGraph & /*nodeGraph*/, const BFFToken * /*funcStartIter*/, const Function * /*function*/ ) override
    {
        ASSERT( false );
        return false;
    }
    virtual bool IsAFile() const override { return true; }

    using Node::FixupPathForVSIntegration;
};
REFLECT_BEGIN( NodeTestHelper, Node, MetaNone() )
REFLECT_END( NodeTestHelper )

// FileNodeTestHelper
//------------------------------------------------------------------------------
class FileNodeTestHelper : public FileNode
{
public:
    using FileNode::DoBuild;
};

// EmptyGraph
//------------------------------------------------------------------------------
void TestGraph::EmptyGraph() const
{
    NodeGraph ng;
}

// TestNodeTypes
//------------------------------------------------------------------------------
void TestGraph::TestNodeTypes() const
{
    FBuild fb;
    NodeGraph ng;

    // Node names differ on Window vs other platforms due to paths etc
#if defined( __WINDOWS__ )
    #define CHOOSE_NAME( WINDOWS_PATH, OTHER_PATH ) AStackString name( WINDOWS_PATH )
#else
    #define CHOOSE_NAME( WINDOWS_PATH, OTHER_PATH ) AStackString name( OTHER_PATH )
#endif

    // Test each node can be created and type mappings are consistent
#define TEST_NODE( TYPE, TYPE_ENUM, FRIENDLY_TYPE, WINDOWS_PATH, OTHER_PATH ) \
    { \
        CHOOSE_NAME( WINDOWS_PATH, OTHER_PATH ); \
        const TYPE * node = ng.CreateNode<TYPE>( name ); \
        TEST_ASSERT( node->GetType() == Node::TYPE_ENUM ); \
        TEST_ASSERT( TYPE::GetTypeS() == Node::TYPE_ENUM ); \
        TEST_ASSERT( AStackString( FRIENDLY_TYPE ) == node->GetTypeName() ); \
    } while ( false )

    // TODO:C - It would be nice to restructure this so that new nodes are automatically tested
    TEST_NODE( FileNode, FILE_NODE, "File", "file", "file" );
    TEST_NODE( CompilerNode, COMPILER_NODE, "Compiler", "c:\\cl.exe", "/usr/bin/gcc" );
    TEST_NODE( CopyFileNode, COPY_FILE_NODE, "CopyFile", "c:\\file", "/path/file" );
    TEST_NODE( DirectoryListNode, DIRECTORY_LIST_NODE, "Directory", "path\\|*.cpp|false|", "path/|*.cpp|false|" );
    TEST_NODE( ExecNode, EXEC_NODE, "Exec", "c:\\exec", "/path/exec" );
    TEST_NODE( LibraryNode, LIBRARY_NODE, "Library", "c:\\library.lib", "/library/library.a" );
    TEST_NODE( ObjectNode, OBJECT_NODE, "Object", "c:\\object.obj", "/path/object.a" );
    TEST_NODE( AliasNode, ALIAS_NODE, "Alias", "alias", "alias" );
    TEST_NODE( DLLNode, DLL_NODE, "DLL", "c:\\lib.dll", "/tmp/lib.so" );
    TEST_NODE( ExeNode, EXE_NODE, "Exe", "c:\\exe.exe", "/tmp/exe.exe" );
    TEST_NODE( UnityNode, UNITY_NODE, "Unity", "unity", "unity" );
    TEST_NODE( CSNode, CS_NODE, "C#", "c:\\csharp.dll", "/path/csharp.dll" );
    TEST_NODE( TestNode, TEST_NODE, "Test", "c:\\output.txt", "/path/output.txt" );
    TEST_NODE( RemoveDirNode, REMOVE_DIR_NODE, "RemoveDir", "remove", "remove" );

#undef TEST_NODE
#undef CHOOSE_NAME
}

// FileNode
//------------------------------------------------------------------------------
void TestGraph::SingleFileNode() const
{
    NodeGraph ng;

    // make sure a node of the name we are going to use doesn't exist
    const AStackString testFileName( "Tools/FBuild/FBuildTest/Data/TestGraph/library.cpp" );
    TEST_ASSERT( ng.FindNode( testFileName ) == nullptr );

    // create the node, and make sure we can access it by name
    FileNode * node = ng.CreateNode<FileNode>( testFileName );
    TEST_ASSERT( ng.FindNode( testFileName ) == node );

    // Manually build a single node
    Job j( node );
    PRAGMA_DISABLE_PUSH_MSVC( 4946 ) // reinterpret_cast used between related classes
    FileNodeTestHelper * helper = reinterpret_cast<FileNodeTestHelper *>( node );
    PRAGMA_DISABLE_POP_MSVC
    TEST_ASSERT( helper->DoBuild( &j ) == Node::BuildResult::eOk );

    // Found file should have a non-zero stamp
    TEST_ASSERT( node->GetStamp() != 0 );
}

// FileNode
//------------------------------------------------------------------------------
void TestGraph::SingleFileNodeMissing() const
{
    NodeGraph ng;

    // make a node for a file that does not exist
    const AStackString testFileName( "ThisFileDoesNotExist.cpp" );
    FileNode * node = ng.CreateNode<FileNode>( testFileName );

    // Manually build a single node
    Job j( node );
    PRAGMA_DISABLE_PUSH_MSVC( 4946 ) // reinterpret_cast used between related classes
    FileNodeTestHelper * helper = reinterpret_cast<FileNodeTestHelper *>( node );
    PRAGMA_DISABLE_POP_MSVC
    TEST_ASSERT( helper->DoBuild( &j ) == Node::BuildResult::eOk );

    // Missing file should have a zero stamp
    TEST_ASSERT( node->GetStamp() == 0 );
}

// TestSerialization
//------------------------------------------------------------------------------
void TestGraph::TestSerialization() const
{
    const char * dbFile1 = "../tmp/Test/Graph/fbuild.db.1";
    const char * dbFile2 = "../tmp/Test/Graph/fbuild.db.2";

    // load the config file and save the resulting db
    {
        // Ensure we're creating the DB by parsing the BFF
        EnsureFileDoesNotExist( AStackString( dbFile1 ) );

        FBuildOptions options;
        options.m_ConfigFile = "fbuild.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile1 ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile1 ) );
        TEST_ASSERT( FileIO::FileExists( dbFile1 ) );
    }

    // load the config, this time from the db, and save it again
    {
        FBuildOptions options;
        options.m_ConfigFile = "fbuild.bff";
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile1 ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile2 ) );
        TEST_ASSERT( FileIO::FileExists( dbFile2 ) );

        // keep working dir active

        // compare the two files
        FileStream fs1;
        FileStream fs2;
        fs1.Open( dbFile1 );
        fs2.Open( dbFile2 );
        TEST_ASSERT( fs1.GetFileSize() == fs2.GetFileSize() ); // size should be the same
        UniquePtr<char, FreeDeletor> buffer1( (char *)ALLOC( MEGABYTE ) );
        UniquePtr<char, FreeDeletor> buffer2( (char *)ALLOC( MEGABYTE ) );
        uint32_t remaining = (uint32_t)fs1.GetFileSize();
        while ( remaining > 0 )
        {
            const uint32_t readNow = Math::Min<uint32_t>( remaining, MEGABYTE );
            TEST_ASSERT( fs1.Read( buffer1.Get(), readNow ) == readNow );
            TEST_ASSERT( fs2.Read( buffer2.Get(), readNow ) == readNow );
            remaining -= readNow;

            // content should be the same
            TEST_ASSERT( AString::StrNCmp( buffer1.Get(), buffer2.Get(), readNow ) == 0 );
        }
    }
}

// TestCleanPath
//------------------------------------------------------------------------------
void TestGraph::TestCleanPath() const
{
    // Change current dir to a known location that exists on all windows machines
    FBuildOptions fo;
#if defined( __WINDOWS__ )
    fo.SetWorkingDir( AStackString( "C:\\Windows\\System32" ) );
#else
    fo.SetWorkingDir( AStackString( "/tmp/subDir" ) );
#endif

    FBuild f( fo );

#if defined( __WINDOWS__ )
    #define CHECK( a, b, c ) \
        { \
            AStackString cleaned; \
            NodeGraph::CleanPath( AStackString( a ), cleaned ); \
            TEST_ASSERT( cleaned == b ); \
        }
#else
    #define CHECK( a, b, c ) \
        { \
            AStackString cleaned; \
            NodeGraph::CleanPath( AStackString( a ), cleaned ); \
            TEST_ASSERT( cleaned == c ); \
        }
#endif

    //   "\..\"
    CHECK( "file.dat", "C:\\Windows\\System32\\file.dat", "/tmp/subDir/file.dat" )
    CHECK( "..\\file.dat", "C:\\Windows\\file.dat", "/tmp/file.dat" )
    CHECK( "..\\..\\file.dat", "C:\\file.dat", "/file.dat" )
    CHECK( "..\\..\\..\\file.dat", "C:\\file.dat", "/file.dat" )
    CHECK( "folder\\..\\", "C:\\Windows\\System32\\", "/tmp/subDir/" )
    CHECK( "folder\\..", "C:\\Windows\\System32\\", "/tmp/subDir/" )

    //   "/../"
    CHECK( "file.dat", "C:\\Windows\\System32\\file.dat", "/tmp/subDir/file.dat" )
    CHECK( "../file.dat", "C:\\Windows\\file.dat", "/tmp/file.dat" )
    CHECK( "../../file.dat", "C:\\file.dat", "/file.dat" )
    CHECK( "../../../file.dat", "C:\\file.dat", "/file.dat" )
    CHECK( "folder/../", "C:\\Windows\\System32\\", "/tmp/subDir/" )
    CHECK( "folder/..", "C:\\Windows\\System32\\", "/tmp/subDir/" )

    //   "\.\"
    CHECK( ".\\file.dat", "C:\\Windows\\System32\\file.dat", "/tmp/subDir/file.dat" )
    CHECK( "folder\\.\\file.dat", "C:\\Windows\\System32\\folder\\file.dat", "/tmp/subDir/folder/file.dat" )
    CHECK( ".\\.\\.\\file.dat", "C:\\Windows\\System32\\file.dat", "/tmp/subDir/file.dat" )
    CHECK( "folder\\.\\", "C:\\Windows\\System32\\folder\\", "/tmp/subDir/folder/" )
    CHECK( "folder\\.", "C:\\Windows\\System32\\folder\\", "/tmp/subDir/folder/" )

    //   "/./"
    CHECK( "./file.dat", "C:\\Windows\\System32\\file.dat", "/tmp/subDir/file.dat" )
    CHECK( "folder/./file.dat", "C:\\Windows\\System32\\folder\\file.dat", "/tmp/subDir/folder/file.dat" )
    CHECK( "./././file.dat", "C:\\Windows\\System32\\file.dat", "/tmp/subDir/file.dat" )
    CHECK( "folder/./", "C:\\Windows\\System32\\folder\\", "/tmp/subDir/folder/" )
    CHECK( "folder/.", "C:\\Windows\\System32\\folder\\", "/tmp/subDir/folder/" )

    //   full path '\'
#if defined( __WINDOWS__ )
    CHECK( "C:\\Windows\\System32\\file.dat", "C:\\Windows\\System32\\file.dat", "" )
    CHECK( "C:\\Windows\\System32\\..\\file.dat", "C:\\Windows\\file.dat", "" )
    CHECK( "C:\\Windows\\System32\\..\\..\\file.dat", "C:\\file.dat", "" )
    CHECK( "C:\\Windows\\System32\\..\\..\\..\\file.dat", "C:\\file.dat", "" )
#endif

    //   full path '/'
#if defined( __WINDOWS__ )
    CHECK( "C:/Windows/System32/file.dat", "C:\\Windows\\System32\\file.dat", "" )
    CHECK( "C:/Windows/System32/../file.dat", "C:\\Windows\\file.dat", "" )
    CHECK( "C:/Windows/System32/../../file.dat", "C:\\file.dat", "" )
    CHECK( "C:/Windows/System32/../../../file.dat", "C:\\file.dat", "" )
#endif

    // files with . in them
    CHECK( ".file.dat", "C:\\Windows\\System32\\.file.dat", "/tmp/subDir/.file.dat" )
    CHECK( ".file", "C:\\Windows\\System32\\.file", "/tmp/subDir/.file" )
    CHECK( "subdir\\.file", "C:\\Windows\\System32\\subdir\\.file", "/tmp/subDir/subdir/.file" )

    // multiple slash removal
    CHECK( "subdir\\\\.file", "C:\\Windows\\System32\\subdir\\.file", "/tmp/subDir/subdir/.file" )
    CHECK( "subdir//.file", "C:\\Windows\\System32\\subdir\\.file", "/tmp/subDir/subdir/.file" )
    CHECK( "subdir//.//.file", "C:\\Windows\\System32\\subdir\\.file", "/tmp/subDir/subdir/.file" )
    CHECK( "subdir\\\\.\\\\.file", "C:\\Windows\\System32\\subdir\\.file", "/tmp/subDir/subdir/.file" )
    CHECK( "subdir\\\\..\\\\.file", "C:\\Windows\\System32\\.file", "/tmp/subDir/.file" )
    CHECK( "subdir//..//.file", "C:\\Windows\\System32\\.file", "/tmp/subDir/.file" )

    // edge cases/regressions
#if defined( __WINDOWS__ )
    // - There was a bug with folders beginning with a slash on Windows
    CHECK( "\\folder\\file", "C:\\Windows\\System32\\folder\\file", "" )
#endif

#undef CHECK
}

// TestPartialCleanPath
//------------------------------------------------------------------------------
void TestGraph::TestCleanPathPartial() const
{
    // Change current dir to a known location that exists on all windows machines
    FBuildOptions fo;
#if defined( __WINDOWS__ )
    fo.SetWorkingDir( AStackString( "C:\\Windows\\System32" ) );
#else
    fo.SetWorkingDir( AStackString( "/tmp/subDir" ) );
#endif

    FBuild f( fo );

#define CHECK( input, expectedOutput, makeFullPath ) \
        do \
        { \
            AStackString cleaned; \
            NodeGraph::CleanPath( AStackString( input ), cleaned, makeFullPath ); \
            TEST_ASSERT( cleaned == expectedOutput ); \
        } while ( false )

#if defined( __WINDOWS__ )
    #define CHECK_RELATIVE( input, expectedWindows, expectedOther ) \
            CHECK( input, expectedWindows, false );
    #define CHECK_FULLPATH( input, expectedWindows, expectedOther ) \
            CHECK( input, expectedWindows, true );
#else
    #define CHECK_RELATIVE( input, expectedWindows, expectedOther ) \
            CHECK( input, expectedOther, false );
    #define CHECK_FULLPATH( input, expectedWindows, expectedOther ) \
            CHECK( input, expectedOther, true );
#endif

    //   "\..\"
    CHECK_RELATIVE( "file.dat", "file.dat", "file.dat" )
    CHECK_RELATIVE( "..\\file.dat", "..\\file.dat", "../file.dat" )
    CHECK_RELATIVE( "..\\..\\file.dat", "..\\..\\file.dat", "../../file.dat" )
    CHECK_RELATIVE( "..\\..\\..\\file.dat", "..\\..\\..\\file.dat", "../../../file.dat" )

    //   "/../"
    CHECK_RELATIVE( "../file.dat", "..\\file.dat", "../file.dat" )
    CHECK_RELATIVE( "../../file.dat", "..\\..\\file.dat", "../../file.dat" )
    CHECK_RELATIVE( "../../../file.dat", "..\\..\\..\\file.dat", "../../../file.dat" )

    //   "\.\"
    CHECK_RELATIVE( ".\\file.dat", "file.dat", "file.dat" )
    CHECK_RELATIVE( "folder\\.\\file.dat", "folder\\file.dat", "folder/file.dat" )
    CHECK_RELATIVE( ".\\.\\.\\file.dat", "file.dat", "file.dat" )

    //   "/./"
    CHECK_RELATIVE( "./file.dat", "file.dat", "file.dat" )
    CHECK_RELATIVE( "folder/./file.dat", "folder\\file.dat", "folder/file.dat" )
    CHECK_RELATIVE( "./././file.dat", "file.dat", "file.dat" )

    // ".." collapsing
    CHECK_RELATIVE( "one\\two\\..\\..\\three\\four\\file.dat", "three\\four\\file.dat", "three/four/file.dat" )
    CHECK_RELATIVE( "one\\two\\..\\three\\file.dat", "one\\three\\file.dat", "one/three/file.dat" )
    CHECK_RELATIVE( "one\\two\\..\\..\\..\\..\\three\\four\\file.dat", "..\\..\\three\\four\\file.dat", "../../three/four/file.dat" )

    //   full path '\'
#if defined( __WINDOWS__ )
    CHECK_RELATIVE( "C:\\Windows\\System32\\file.dat", "C:\\Windows\\System32\\file.dat", "" )
    CHECK_RELATIVE( "C:\\Windows\\System32\\..\\file.dat", "C:\\Windows\\file.dat", "" )
    CHECK_RELATIVE( "C:\\Windows\\System32\\..\\..\\file.dat", "C:\\file.dat", "" )
    CHECK_RELATIVE( "C:\\Windows\\System32\\..\\..\\..\\file.dat", "C:\\file.dat", "" )
#endif

    //   full path '/'
#if defined( __WINDOWS__ )
    CHECK_RELATIVE( "C:/Windows/System32/file.dat", "C:\\Windows\\System32\\file.dat", "" )
    CHECK_RELATIVE( "C:/Windows/System32/../file.dat", "C:\\Windows\\file.dat", "" )
    CHECK_RELATIVE( "C:/Windows/System32/../../file.dat", "C:\\file.dat", "" )
    CHECK_RELATIVE( "C:/Windows/System32/../../../file.dat", "C:\\file.dat", "" )
#endif

    // files with . in them
    CHECK_RELATIVE( ".file.dat", ".file.dat", ".file.dat" )
    CHECK_RELATIVE( ".file", ".file", ".file" )
    CHECK_RELATIVE( "subdir\\.file", "subdir\\.file", "subdir/.file" )

    // multiple slash removal
    CHECK_RELATIVE( "subdir\\\\.file", "subdir\\.file", "subdir/.file" )
    CHECK_RELATIVE( "subdir//.file", "subdir\\.file", "subdir/.file" )
    CHECK_RELATIVE( "subdir//.//.file", "subdir\\.file", "subdir/.file" )
    CHECK_RELATIVE( "subdir\\\\.\\\\.file", "subdir\\.file", "subdir/.file" )
    CHECK_RELATIVE( "subdir\\\\..\\\\.file", ".file", ".file" )
    CHECK_RELATIVE( "subdir//..//.file", ".file", ".file" )

    // edge cases/regressions
#if defined( __WINDOWS__ )
        // - There was a bug with folders beginning with a slash on Windows
    CHECK_RELATIVE( "\\folder\\file", "folder\\file", "" )
#endif
    // - A bug meant paths terminated with .. were not correctly handled
    CHECK_FULLPATH( "..", "C:\\Windows\\", "/tmp/" )
    CHECK_FULLPATH( ".\\..", "C:\\Windows\\", "/tmp/" )
    CHECK_FULLPATH( "./..", "C:\\Windows\\", "/tmp/" )

#undef CHECK_FULLPATH
#undef CHECK_RELATIVE
#undef CHECK
}

// TestDeepGraph
//------------------------------------------------------------------------------
void TestGraph::TestDeepGraph() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestGraph/DeepGraph.bff";
    options.m_NumWorkerThreads = 1;

    const char * dbFile1 = "../tmp/Test/Graph/DeepGraph.fdb";

    {
        // do a clean build
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "all" ) );

        // save the DB
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile1 ) );
        TEST_ASSERT( FileIO::FileExists( dbFile1 ) );
    }

    {
        const Timer t;

        // no op build
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile1 ) );
        TEST_ASSERT( fBuild.Build( "all" ) );
        CheckStatsNode( 1, 0, Node::OBJECT_NODE );

        // make sure walking the graph wasn't slow (should be a good deal less
        // than 100ms, but allow for a lot of slack on the test machine)
        TEST_ASSERT( t.GetElapsed() < 2.0f );
    }
}

// TestNoStopOnFirstError
//------------------------------------------------------------------------------
void TestGraph::TestNoStopOnFirstError() const
{
    FBuildTestOptions options;
    options.m_NumWorkerThreads = 0; // ensure test behaves deterministically
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestGraph/NoStopOnFirstError/fbuild.bff";

    // "Stop On First Error" build (default behaviour)
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "all" ) == false ); // Expect build to fail

        // Check stats: Seen, Built, Type
        CheckStatsNode( 4, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 0, Node::LIBRARY_NODE );
        CheckStatsNode( 1, 0, Node::ALIAS_NODE );

        // One node should have failed
        const FBuildStats::Stats & nodeStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( nodeStats.m_NumFailed == 1 );
    }

    // "No Stop On First Error" build
    options.m_StopOnFirstError = false;
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( "all" ) == false ); // Expect build to fail

        // Check stats: Seen, Built, Type
        CheckStatsNode( 4, 0, Node::OBJECT_NODE );
        CheckStatsNode( 2, 0, Node::LIBRARY_NODE );
        CheckStatsNode( 1, 0, Node::ALIAS_NODE );

        // Add 4 nodes should have failed
        const FBuildStats::Stats & nodeStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( nodeStats.m_NumFailed == 4 );
    }
}

// DBLocationChanged
//------------------------------------------------------------------------------
void TestGraph::DBLocationChanged() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestGraph/DatabaseMoved/fbuild.bff";

    const char * dbFile1 = "../tmp/Test/Graph/DatabaseMoved/1/GraphMoved.fdb";
    const char * dbFile2 = "../tmp/Test/Graph/DatabaseMoved/2/GraphMoved.fdb";

    EnsureFileDoesNotExist( dbFile1 );
    EnsureFileDoesNotExist( dbFile2 );

    // Create a DB
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile1 ) );

        // Copy the DB
        AStackString dbPath2( dbFile2 );
        dbPath2.SetLength( (uint32_t)( dbPath2.FindLast( FORWARD_SLASH ) - dbPath2.Get() ) );
        TEST_ASSERT( FileIO::EnsurePathExists( dbPath2 ) );
        TEST_ASSERT( FileIO::FileCopy( dbFile1, dbFile2 ) );
    }

    // Moving a DB should result in a message and a failed build
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile2 ) == false );
        TEST_ASSERT( GetRecordedOutput().Find( "Database has been moved" ) );
    }

    // With -continueafterdmove, message should be emitted, but build should pass
    options.m_ContinueAfterDBMove = true;
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile2 ) == true );
        TEST_ASSERT( AStackString( GetRecordedOutput() ).Replace( "Database has been moved", "", 2 ) == 2 ); // Find twice
    }
}

// DBCorrupt
//------------------------------------------------------------------------------
void TestGraph::DBCorrupt() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestGraph/DatabaseCorrupt/fbuild.bff";

    // We'll save a valid DB, corrupt it and ensure that's detected
    const char * dbFile = "../tmp/Test/Graph/DatabaseCorrupt/fbuild.fdb";
    const char * dbFileCorrupt = "../tmp/Test/Graph/DatabaseCorrupt/fbuild.fdb.corrupt";

    // Clear all copies of the DB first
    EnsureFileDoesNotExist( dbFile );
    EnsureFileDoesNotExist( dbFileCorrupt );

    // Test corruption at various places in the file
    static_assert( sizeof( NodeGraphHeader ) == 16, "Update test for DB format change" );
    // clang-format off
    static const uint32_t corruptionOffsets[] =
    {
        0,      // Header - magic identifier
        8,      // Header - hash of content
        128,    // Arbitrary position in the file
    };
    // clang-format on

    // Corrupt the DB
    for ( const uint32_t corruptionOffset : corruptionOffsets )
    {
        // Create a DB
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );
            TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );
        }

        // Corrupt the DB
        {
            FileStream f;

            // Read DB into memory
            TEST_ASSERT( f.Open( dbFile, FileStream::READ_ONLY ) );
            AString buffer;
            buffer.SetLength( (uint32_t)f.GetFileSize() );
            TEST_ASSERT( f.ReadBuffer( buffer.Get(), f.GetFileSize() ) == f.GetFileSize() );
            f.Close(); // Explicit close so we can re-open

            // Corrupt it by flipping some bits
            buffer[ corruptionOffset ] = ~buffer[ corruptionOffset ];

            // Save corrupt DB
            TEST_ASSERT( f.Open( dbFile, FileStream::WRITE_ONLY ) );
            TEST_ASSERT( f.WriteBuffer( buffer.Get(), buffer.GetLength() ) );
        }

        // Initialization should report a warning, but still work
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize( dbFile ) == true );
            TEST_ASSERT( GetRecordedOutput().Find( "Database corrupt" ) );

            // Backup of corrupt DB should exit
            EnsureFileExists( dbFileCorrupt );
        }
    }
}

// BFFDirtied
//------------------------------------------------------------------------------
void TestGraph::BFFDirtied() const
{
    const char * originalBFF = "Tools/FBuild/FBuildTest/Data/TestGraph/BFFDirtied/fbuild.bff";
    const char * copyOfBFF = "../tmp/Test/Graph/BFFDirtied/fbuild.bff";
    const char * dbFile = "../tmp/Test/Graph/BFFDirtied/fbuild.fdb";

    EnsureFileDoesNotExist( copyOfBFF );
    EnsureFileDoesNotExist( dbFile );

    // Ensure test output dir exists
    {
        AStackString copyOfBFFPath( copyOfBFF );
        copyOfBFFPath.SetLength( (uint32_t)( copyOfBFFPath.FindLast( FORWARD_SLASH ) - copyOfBFFPath.Get() ) );
        TEST_ASSERT( FileIO::EnsurePathExists( copyOfBFFPath ) );
    }

    // Copy main BFF and included BFF
    {
        TEST_ASSERT( FileIO::FileCopy( originalBFF, copyOfBFF ) );
        TEST_ASSERT( FileIO::SetReadOnly( copyOfBFF, false ) );
    }

    FBuildOptions options;
    options.m_ConfigFile = copyOfBFF;

    // Load from copy of BFF
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );

        // Ensure Settings() are being read in, since tests below
        // are verifying they don't persist when the BFF changes
        TEST_ASSERT( fBuild.GetSettings()->GetCachePath().IsEmpty() == false );
        TEST_ASSERT( fBuild.GetEnvironmentStringSize() > 0 );
        TEST_ASSERT( fBuild.GetSettings()->GetWorkerList().IsEmpty() == false );
    }

    // Modify file, ensuring filetime has changed (different file systems have different resolutions)
    const uint64_t originalTime = FileIO::GetFileLastWriteTime( AStackString( copyOfBFF ) );
    const Timer t;
    uint32_t sleepTimeMS = 2;
    for ( ;; )
    {
        // Truncate file
        FileStream fs;
        TEST_ASSERT( fs.Open( copyOfBFF, FileStream::WRITE_ONLY ) );
        fs.Close();

        // See if the mod time has changed
        if ( FileIO::GetFileLastWriteTime( AStackString( copyOfBFF ) ) != originalTime )
        {
            break; // All done
        }

        // Wait a while and try again
        Thread::Sleep( sleepTimeMS );
        sleepTimeMS = Math::Max<uint32_t>( sleepTimeMS * 2, 128 );

        TEST_ASSERT( t.GetElapsed() < 10.0f ); // Sanity check fail test after a longtime
    }

    // Modify BFF (make it empty)
    {
        FileStream fs;
        TEST_ASSERT( fs.Open( copyOfBFF, FileStream::WRITE_ONLY ) );
    }

    // Load from dirtied BFF
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) );

        // Ensure user was informed of reparsing trigger
        TEST_ASSERT( GetRecordedOutput().Find( "has changed (reparsing will occur)" ) );

        // Get cache path directly from property to ignore environment variables
        const ReflectionInfo * ri = fBuild.GetSettings()->GetReflectionInfoV();
        AStackString cachePath;
        TEST_ASSERT( ri->GetProperty( (void *)fBuild.GetSettings(), "CachePath", &cachePath ) );

        // Make sure settings don't "leak" from the original BFF into the new one
        TEST_ASSERT( cachePath.IsEmpty() );
        TEST_ASSERT( fBuild.GetEnvironmentStringSize() == 0 );
        TEST_ASSERT( fBuild.GetSettings()->GetWorkerList().IsEmpty() );
    }
}

// DBVersionChanged
//------------------------------------------------------------------------------
void TestGraph::DBVersionChanged() const
{
    // Generate a fake old version headers
    NodeGraphHeader header;
    header.SetContentHash( xxHash3::Calc64( "", 0 ) );
    MemoryStream ms;
    ms.WriteBuffer( &header, sizeof( header ) );

    // Since we're poking this, we want to know if the layout ever changes somehow
    TEST_ASSERT( ms.GetFileSize() == 16 );
    TEST_ASSERT( ( (const uint8_t *)ms.GetDataMutable() )[ 3 ] == NodeGraphHeader::kCurrentVersion );

    ( (uint8_t *)ms.GetDataMutable() )[ 3 ] = ( NodeGraphHeader::kCurrentVersion - 1 );

    const char * oldDB = "../tmp/Test/Graph/DBVersionChanged/fbuild.fdb";
    const char * emptyBFF = "../tmp/Test/Graph/DBVersionChanged/fbuild.bff";

    FBuildTestOptions options;
    options.m_ConfigFile = emptyBFF;
    FBuild fBuild( options );

    // cleanup & prep
    {
        AStackString oldDBPath( oldDB );
        oldDBPath.SetLength( (uint32_t)( oldDBPath.FindLast( FORWARD_SLASH ) - oldDBPath.Get() ) );
        TEST_ASSERT( FileIO::EnsurePathExists( oldDBPath ) );

        // write old DB to disk
        FileStream fs;
        TEST_ASSERT( fs.Open( oldDB, FileStream::WRITE_ONLY ) );
        TEST_ASSERT( fs.WriteBuffer( ms.GetData(), ms.GetFileSize() ) );

        // create an empty bff
        FileStream fs2;
        TEST_ASSERT( fs2.Open( emptyBFF, FileStream::WRITE_ONLY ) );
    }

    // Init from old DB
    TEST_ASSERT( fBuild.Initialize( oldDB ) );

    // Ensure user was informed about change
    TEST_ASSERT( GetRecordedOutput().Find( "Database version has changed" ) );
}

// FixupErrorPaths
//------------------------------------------------------------------------------
void TestGraph::FixupErrorPaths() const
{
    // Use a known location we can test for
#if defined( __WINDOWS__ )
    const AStackString workingDir( "C:\\Windows\\System32" );
#else
    const AStackString workingDir( "/tmp/subDir" );
#endif

    // FBuild is used during path cleaning to access working dir
    FBuildOptions fo;
    fo.SetWorkingDir( workingDir );
    FBuild f( fo );

    // Helper macro
    AStackString fixup;
    AStackString original;
#define TEST_FIXUP( path ) \
        original = path; \
        fixup = path; \
        NodeTestHelper::FixupPathForVSIntegration( fixup ); \
        do \
        { \
            if ( ( original.Find( "/mnt/" ) == nullptr ) && \
                 ( fixup.BeginsWith( workingDir ) == false ) ) \
            { \
                TEST_ASSERTM( false, "Path was not fixed up as expected.\n" \
                                     "Original           : %s\n" \
                                     "Returned           : %s\n" \
                                     "Expected BeginsWith: %s\n", \
                                     original.Get(), \
                                     fixup.Get(), \
                                     workingDir.Get() ); \
            } \
            else if ( fixup.Find( "/mnt/" ) != nullptr ) \
            { \
                TEST_ASSERTM( false, "Path was not fixed up as expected.\n" \
                                     "Original           : %s\n" \
                                     "Returned           : %s\n" \
                                     "Unexpected         : Contains '/mnt/'\n", \
                                     original.Get(), \
                                     fixup.Get() ); \
            } \
        } while ( false )

    // GCC/Clang style
    TEST_FIXUP( "Core/Mem/Mem.h:23:1: warning: some warning text" );
    TEST_FIXUP( ".\\Tools/FBuild/FBuildCore/Graph/Node.h(294,24): warning: some warning text" );

    // SNC style
    TEST_FIXUP( "Core/Mem/Mem.h(23,1): warning 55: some warning text" );

    // VBCC Style
    TEST_FIXUP( "warning 55 in line 23 of \"Core/Mem/Mem.h\": some warning text" );

    // WSL
    TEST_FIXUP( "/mnt/c/p4/depot/Code/Core/Mem/Mem.h:23:1: warning: some warning text" );

#undef TEST_FIXUP
}

// CyclicDependency
//------------------------------------------------------------------------------
void TestGraph::CyclicDependency() const
{
    // Statically defined cyclic dependencies are detected at BFF parse time,
    // but additional ones can be created at build time, so have to be detected
    // at build time.
    //
    // This test runs a build step that outputs an output into its own source
    // directory the first time it has been run, so that the second time it is
    // run there is a cyclic dependency.

    const char * const bffFile = "Tools/FBuild/FBuildTest/Data/TestGraph/CyclicDependency/fbuild.bff";
    const char * const dbFile = "../tmp/Test/Graph/CyclicDependency/fbuild.db";

    FBuildTestOptions options;
    options.m_ConfigFile = bffFile;

    // Delete the file if this test has been run before, so that the test is consistent
    FileIO::FileDelete( "../tmp/Test/Graph/CyclicDependency/file.x" );

    // First run
    {
        // Initialization is ok because the problem occurs at build time
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() == true );

        // First build passes, but outputs data into the source dir that is a problem next time
        TEST_ASSERT( fBuild.Build( "all" ) );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile ) );
    }

    // Second run
    {
        // Initialize
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile ) == true );

        // Second build detects the bad dependency created by the first invocation
        // and fails
        TEST_ASSERT( fBuild.Build( "all" ) == false );
        TEST_ASSERT( GetRecordedOutput().Find( "Error: Cyclic dependency detected" ) );
    }
}

// DBLocation
//------------------------------------------------------------------------------
void TestGraph::DBLocation() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestGraph/DatabaseLocation/fbuild.bff";
    options.m_SaveDBOnCompletion = true;

    AString dbFileDefaultLocation( "Tools/FBuild/FBuildTest/Data/TestGraph/DatabaseLocation/" );
    AString dbFileDefaultLocationFullPath( dbFileDefaultLocation );
#if defined( __WINDOWS__ )
    dbFileDefaultLocationFullPath += "fbuild.windows.fdb";
#elif defined( __LINUX__ )
    dbFileDefaultLocationFullPath += "fbuild.linux.fdb";
#elif defined( __OSX__ )
    dbFileDefaultLocationFullPath += "fbuild.osx.fdb";
#endif
    AString dbFileExplicitLocation( "../tmp/Test/Graph/DatabaseLocation/GraphDB.fdb" );

    EnsureFileDoesNotExist( dbFileDefaultLocationFullPath );
    EnsureFileDoesNotExist( dbFileExplicitLocation );

    // Build a target and let serialization save to default location
    {
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        TEST_ASSERT( fBuild.GetDependencyGraphFile() == dbFileDefaultLocationFullPath );

        TEST_ASSERT( fBuild.Build( "TestTarget" ) );
        EnsureFileExists( dbFileDefaultLocationFullPath );
    }

    // Build a target and let serialization save to explicitly specified location
    {
        options.m_DBFile = dbFileExplicitLocation;

        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        const AString & dbFile( fBuild.GetDependencyGraphFile() );
        EnsureFileDoesNotExist( dbFile );

        TEST_ASSERT( fBuild.Build( "TestTarget" ) );
        TEST_ASSERT( PathUtils::ArePathsEqual( dbFile, dbFileExplicitLocation ) );
        EnsureFileExists( dbFileDefaultLocationFullPath );
    }
}

//------------------------------------------------------------------------------
