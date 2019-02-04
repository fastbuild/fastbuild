// TestGraph.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CSNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"

// system
#if defined( __WINDOWS__ )
    #include <windows.h>
#endif

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
    void TestDirectoryListNode() const;
    void TestSerialization() const;
    void TestDeepGraph() const;
    void TestNoStopOnFirstError() const;
    void DBLocationChanged() const;
    void BFFDirtied() const;
    void DBVersionChanged() const;
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
    REGISTER_TEST( TestDirectoryListNode )
    REGISTER_TEST( TestSerialization )
    REGISTER_TEST( TestDeepGraph )
    REGISTER_TEST( TestNoStopOnFirstError )
    REGISTER_TEST( DBLocationChanged )
    REGISTER_TEST( BFFDirtied )
    REGISTER_TEST( DBVersionChanged )
REGISTER_TESTS_END

// EmptyGraph
//------------------------------------------------------------------------------
void TestGraph::EmptyGraph() const
{
    NodeGraph * ng = FNEW( NodeGraph );
    FDELETE ng;
}

// TestNodeTypes
//------------------------------------------------------------------------------
void TestGraph::TestNodeTypes() const
{
    FBuild fb;
    NodeGraph ng;

    FileNode * fn = ng.CreateFileNode( AStackString<>( "file" ) );
    TEST_ASSERT( fn->GetType() == Node::FILE_NODE);
    TEST_ASSERT( FileNode::GetTypeS() == Node::FILE_NODE);

    CompilerNode * cn( nullptr );
    {
        #if defined( __WINDOWS__ )
            cn = ng.CreateCompilerNode( AStackString<>( "c:\\cl.exe" ) );
        #else
            cn = ng.CreateCompilerNode( AStackString<>( "/usr/bin/gcc" ) );
        #endif
        TEST_ASSERT( cn->GetType() == Node::COMPILER_NODE );
        TEST_ASSERT( AStackString<>( "Compiler" ) == cn->GetTypeName() );
    }

    {
        #if defined( __WINDOWS__ )
            Node * n = ng.CreateCopyFileNode( AStackString<>( "c:\\dummy" ) );
        #else
            Node * n = ng.CreateCopyFileNode( AStackString<>( "/dummy/dummy" ) );
        #endif
        TEST_ASSERT( n->GetType() == Node::COPY_FILE_NODE );
        TEST_ASSERT( CopyFileNode::GetTypeS() == Node::COPY_FILE_NODE );
        TEST_ASSERT( AStackString<>( "CopyFile" ) == n->GetTypeName() );
    }

    #if defined( __WINDOWS__ )
        DirectoryListNode * dn = ng.CreateDirectoryListNode( AStackString<>( "path\\|*.cpp|false|" ) );
    #else
        DirectoryListNode * dn = ng.CreateDirectoryListNode( AStackString<>( "path/|*.cpp|false|" ) );
    #endif
    TEST_ASSERT( dn->GetType() == Node::DIRECTORY_LIST_NODE );
    TEST_ASSERT( DirectoryListNode::GetTypeS() == Node::DIRECTORY_LIST_NODE );
    TEST_ASSERT( AStackString<>( "Directory" ) == dn->GetTypeName() );

    {
        #if defined( __WINDOWS__ )
            Node * n = ng.CreateExecNode( AStackString<>( "c:\\execdummy" ) );
        #else
            Node * n = ng.CreateExecNode( AStackString<>( "/execdummy/execdummy" ) );
        #endif
        TEST_ASSERT( n->GetType() == Node::EXEC_NODE );
        TEST_ASSERT( ExecNode::GetTypeS() == Node::EXEC_NODE );
        TEST_ASSERT( AStackString<>( "Exec" ) == n->GetTypeName() );
    }
    {
        #if defined( __WINDOWS__ )
            Node * n = ng.CreateLibraryNode( AStackString<>( "c:\\library.lib" ) );
        #else
            Node * n = ng.CreateLibraryNode( AStackString<>( "/library/library.a" ) );
        #endif
        TEST_ASSERT( n->GetType() == Node::LIBRARY_NODE );
        TEST_ASSERT( LibraryNode::GetTypeS() == Node::LIBRARY_NODE );
        TEST_ASSERT( AStackString<>( "Library" ) == n->GetTypeName() );
    }
    {
        #if defined( __WINDOWS__ )
            Node * n = ng.CreateObjectNode( AStackString<>( "c:\\object.lib" ) );
        #else
            Node * n = ng.CreateObjectNode( AStackString<>( "/library/object.o" ) );
        #endif
        TEST_ASSERT( n->GetType() == Node::OBJECT_NODE );
        TEST_ASSERT( ObjectNode::GetTypeS() == Node::OBJECT_NODE );
        TEST_ASSERT( AStackString<>( "Object" ) == n->GetTypeName() );
    }
    {
        Node * n = ng.CreateAliasNode( AStackString<>( "alias" ) );
        TEST_ASSERT( n->GetType() == Node::ALIAS_NODE );
        TEST_ASSERT( AliasNode::GetTypeS() == Node::ALIAS_NODE );
        TEST_ASSERT( AStackString<>( "Alias" ) == n->GetTypeName() );
    }
    {
        #if defined( __WINDOWS__ )
            AStackString<> dllName( "c:\\lib.dll" );
        #else
            AStackString<> dllName( "/tmp/lib.so" );
        #endif
        Node * n = ng.CreateDLLNode( dllName );
        TEST_ASSERT( n->GetType() == Node::DLL_NODE );
        TEST_ASSERT( DLLNode::GetTypeS() == Node::DLL_NODE );
        TEST_ASSERT( AStackString<>( "DLL" ) == n->GetTypeName() );
    }
    {
        #if defined( __WINDOWS__ )
            AStackString<> exeName( "c:\\exe.exe" );
        #else
            AStackString<> exeName( "/tmp/exe.exe" );
        #endif
        Node * n = ng.CreateExeNode( exeName );
        TEST_ASSERT( n->GetType() == Node::EXE_NODE );
        TEST_ASSERT( ExeNode::GetTypeS() == Node::EXE_NODE );
        TEST_ASSERT( AStackString<>( "Exe" ) == n->GetTypeName() );
    }
    {
        Node * n = ng.CreateUnityNode( AStackString<>( "Unity" ) );
        TEST_ASSERT( n->GetType() == Node::UNITY_NODE);
        TEST_ASSERT( UnityNode::GetTypeS() == Node::UNITY_NODE );
        TEST_ASSERT( AStackString<>( "Unity" ) == n->GetTypeName() );
    }
    {
        #if defined( __WINDOWS__ )
            Node * n = ng.CreateCSNode( AStackString<>( "c:\\csharp.dll" ) );
        #else
            Node * n = ng.CreateCSNode( AStackString<>( "/dummy/csharp.dll" ) );
        #endif
        TEST_ASSERT( n->GetType() == Node::CS_NODE);
        TEST_ASSERT( CSNode::GetTypeS() == Node::CS_NODE );
        TEST_ASSERT( AStackString<>( "C#" ) == n->GetTypeName() );
    }
}

// FileNode
//------------------------------------------------------------------------------
void TestGraph::SingleFileNode() const
{
    FBuild fb;
    NodeGraph ng;

    // make sure a node of the name we are going to use doesn't exist
    const AStackString<> testFileName( "SimpleLibrary/library.cpp" );
    TEST_ASSERT( ng.FindNode( testFileName ) == nullptr );

    // create the node, and make sure we can access it by name
    FileNode * node = ng.CreateFileNode( testFileName );
    TEST_ASSERT( ng.FindNode( testFileName ) == node );

    TEST_ASSERT( fb.Build( node ) );
}

// FileNode
//------------------------------------------------------------------------------
void TestGraph::SingleFileNodeMissing() const
{
    // suppress error output for this test (as the errors are expected)
    FBuildOptions options;
    options.m_ShowErrors = false;

    FBuild fb( options );
    NodeGraph ng;

    // make a node for a file that does not exist
    const AStackString<> testFileName( "ThisFileDoesNotExist.cpp" );
    FileNode * node = ng.CreateFileNode( testFileName );

    // make sure build still passes
    // a missing file is not an error.  it would need to be required by something
    // (like an objectNode which would handle the failure itself)
    TEST_ASSERT( fb.Build( node ) == true );
}

// TestDirectoryListNode
//------------------------------------------------------------------------------
void TestGraph::TestDirectoryListNode() const
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
    patterns.Append( AStackString<>( "library.*" ) );
    DirectoryListNode::FormatName( testFolder,
                                   &patterns,
                                   true, // recursive
                                   Array< AString >(), // excludePaths,
                                   Array< AString >(), // excludeFiles,
                                   Array< AString >(), // excludePatterns,
                                   name );

    // create the node, and make sure we can access it by name
    DirectoryListNode * node = ng.CreateDirectoryListNode( name );
    node->m_Path = testFolder;
    node->m_Patterns = patterns;
    BFFIterator iter;
    TEST_ASSERT( node->Initialize( ng, iter, nullptr ) );
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

// TestSerialization
//------------------------------------------------------------------------------
void TestGraph::TestSerialization() const
{
    const char * dbFile1    = "../tmp/Test/Graph/fbuild.db.1";
    const char * dbFile2    = "../tmp/Test/Graph/fbuild.db.2";

    // load the config file and save the resulting db
    {
        // Ensure we're creating the DB by parsing the BFF
        EnsureFileDoesNotExist( AStackString<>( dbFile1 ) );

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
        AutoPtr< char > buffer1( (char *)ALLOC( MEGABYTE ) );
        AutoPtr< char > buffer2( (char *)ALLOC( MEGABYTE ) );
        uint32_t remaining = (uint32_t)fs1.GetFileSize();
        while ( remaining > 0 )
        {
            const uint32_t readNow = Math::Min< uint32_t >( remaining, MEGABYTE );
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
        fo.SetWorkingDir( AStackString<>( "C:\\Windows\\System32" ) );
    #else
        fo.SetWorkingDir( AStackString<>( "/tmp/subDir" ) );
    #endif

    FBuild f( fo );

    #if defined( __WINDOWS__ )
        #define CHECK(a, b, c) \
        { \
            AStackString<> cleaned; \
            NodeGraph::CleanPath( AStackString<>( a ), cleaned ); \
            TEST_ASSERT( cleaned == b ); \
        }
    #else
        #define CHECK(a, b, c) \
        { \
            AStackString<> cleaned; \
            NodeGraph::CleanPath( AStackString<>( a ), cleaned ); \
            TEST_ASSERT( cleaned == c ); \
        }
    #endif

    //   "\..\"
    CHECK( "file.dat",              "C:\\Windows\\System32\\file.dat",  "/tmp/subDir/file.dat" )
    CHECK( "..\\file.dat",          "C:\\Windows\\file.dat",            "/tmp/file.dat" )
    CHECK( "..\\..\\file.dat",      "C:\\file.dat",                     "/file.dat" )
    CHECK( "..\\..\\..\\file.dat",  "C:\\file.dat",                     "/file.dat" )

    //   "/../"
    CHECK( "file.dat",              "C:\\Windows\\System32\\file.dat",  "/tmp/subDir/file.dat" )
    CHECK( "../file.dat",           "C:\\Windows\\file.dat",            "/tmp/file.dat" )
    CHECK( "../../file.dat",        "C:\\file.dat",                     "/file.dat" )
    CHECK( "../../../file.dat",     "C:\\file.dat",                     "/file.dat" )

    //   "\.\"
    CHECK( ".\\file.dat",           "C:\\Windows\\System32\\file.dat",          "/tmp/subDir/file.dat" )
    CHECK( "folder\\.\\file.dat",   "C:\\Windows\\System32\\folder\\file.dat",  "/tmp/subDir/folder/file.dat" )
    CHECK( ".\\.\\.\\file.dat",     "C:\\Windows\\System32\\file.dat",          "/tmp/subDir/file.dat" )

    //   "/./"
    CHECK( "./file.dat",            "C:\\Windows\\System32\\file.dat",          "/tmp/subDir/file.dat" )
    CHECK( "folder/./file.dat",     "C:\\Windows\\System32\\folder\\file.dat",  "/tmp/subDir/folder/file.dat" )
    CHECK( "./././file.dat",        "C:\\Windows\\System32\\file.dat",          "/tmp/subDir/file.dat" )

    //   full path '\'
    #if defined( __WINDOWS__ )
        CHECK( "C:\\Windows\\System32\\file.dat",               "C:\\Windows\\System32\\file.dat",  "" )
        CHECK( "C:\\Windows\\System32\\..\\file.dat",           "C:\\Windows\\file.dat",            "" )
        CHECK( "C:\\Windows\\System32\\..\\..\\file.dat",       "C:\\file.dat",                     "" )
        CHECK( "C:\\Windows\\System32\\..\\..\\..\\file.dat",   "C:\\file.dat",                     "" )
    #endif

    //   full path '/'
    #if defined( __WINDOWS__ )
        CHECK( "C:/Windows/System32/file.dat",              "C:\\Windows\\System32\\file.dat",      "" )
        CHECK( "C:/Windows/System32/../file.dat",           "C:\\Windows\\file.dat",                "" )
        CHECK( "C:/Windows/System32/../../file.dat",        "C:\\file.dat",                         "" )
        CHECK( "C:/Windows/System32/../../../file.dat",     "C:\\file.dat",                         "" )
    #endif

    // files with . in them
    CHECK( ".file.dat",     "C:\\Windows\\System32\\.file.dat",     "/tmp/subDir/.file.dat" )
    CHECK( ".file",         "C:\\Windows\\System32\\.file",         "/tmp/subDir/.file" )
    CHECK( "subdir\\.file", "C:\\Windows\\System32\\subdir\\.file", "/tmp/subDir/subdir/.file" )

    // multiple slash removal
    CHECK( "subdir\\\\.file",       "C:\\Windows\\System32\\subdir\\.file",     "/tmp/subDir/subdir/.file" )
    CHECK( "subdir//.file",         "C:\\Windows\\System32\\subdir\\.file",     "/tmp/subDir/subdir/.file" )
    CHECK( "subdir//.//.file",      "C:\\Windows\\System32\\subdir\\.file",     "/tmp/subDir/subdir/.file" )
    CHECK( "subdir\\\\.\\\\.file",  "C:\\Windows\\System32\\subdir\\.file",     "/tmp/subDir/subdir/.file" )
    CHECK( "subdir\\\\..\\\\.file", "C:\\Windows\\System32\\.file",             "/tmp/subDir/.file" )
    CHECK( "subdir//..//.file",     "C:\\Windows\\System32\\.file",             "/tmp/subDir/.file" )

    // edge cases/regressions
    #if defined( __WINDOWS__ )
        // - There was a bug with folders beginning with a slash on Windows
        CHECK( "\\folder\\file",    "C:\\Windows\\System32\\folder\\file",      "" )
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
        fo.SetWorkingDir( AStackString<>( "C:\\Windows\\System32" ) );
    #else
        fo.SetWorkingDir( AStackString<>( "/tmp/subDir" ) );
    #endif

    FBuild f( fo );

    #define CHECK( input, expectedOutput, makeFullPath ) \
        { \
            AStackString<> cleaned; \
            NodeGraph::CleanPath( AStackString<>( input ), cleaned, makeFullPath ); \
            TEST_ASSERT( cleaned == expectedOutput ); \
        }

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
    options.m_UseCacheRead = true;
    options.m_UseCacheWrite = true;

    const char * dbFile1 = "../tmp/Test/Graph/DeepGraph.fdb";

    {
        // do a clean build
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( AStackString<>( "all" ) ) );

        // save the DB
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile1 ) );
        TEST_ASSERT( FileIO::FileExists( dbFile1 ) );
    }

    {
        Timer t;

        // no op build
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile1 ) );
        TEST_ASSERT( fBuild.Build( AStackString<>( "all" ) ) );
        CheckStatsNode ( 30,        0,      Node::OBJECT_NODE );

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
    options.m_FastCancel = true;

    // "Stop On First Error" build (default behaviour)
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( AStackString<>( "all" ) ) == false ); // Expect build to fail

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 4,     0,      Node::OBJECT_NODE );
        CheckStatsNode ( 2,     0,      Node::LIBRARY_NODE );
        CheckStatsNode ( 1,     0,      Node::ALIAS_NODE );

        // One node should have failed
        const FBuildStats::Stats & nodeStats = fBuild.GetStats().GetStatsFor( Node::OBJECT_NODE );
        TEST_ASSERT( nodeStats.m_NumFailed == 1 );
    }

    // "No Stop On First Error" build
    options.m_StopOnFirstError = false;
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.Build( AStackString<>( "all" ) ) == false ); // Expect build to fail

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 4,     0,      Node::OBJECT_NODE );
        CheckStatsNode ( 2,     0,      Node::LIBRARY_NODE );
        CheckStatsNode ( 1,     0,      Node::ALIAS_NODE );

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

    const char* dbFile1 = "../tmp/Test/Graph/DatabaseMoved/1/GraphMoved.fdb";
    const char* dbFile2 = "../tmp/Test/Graph/DatabaseMoved/2/GraphMoved.fdb";

    EnsureFileDoesNotExist( dbFile1 );
    EnsureFileDoesNotExist( dbFile2 );

    // Create a DB
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );
        TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile1 ) );

        // Copy the DB
        AStackString<> dbPath2( dbFile2 );
        dbPath2.SetLength( (uint32_t)( dbPath2.FindLast( FORWARD_SLASH ) - dbPath2.Get() ) );
        TEST_ASSERT( FileIO::EnsurePathExists( dbPath2 ) );
        TEST_ASSERT( FileIO::FileCopy( dbFile1, dbFile2 ) );
    }

    // Check that the DB in the new location is detected as invalid and the user
    // is notified appropriately
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize( dbFile2 ) == false );
        TEST_ASSERT( GetRecordedOutput().Find( "Database has been moved" ) );
    }
}

// BFFDirtied
//------------------------------------------------------------------------------
void TestGraph::BFFDirtied() const
{
    const char* originalBFF             = "Tools/FBuild/FBuildTest/Data/TestGraph/BFFDirtied/fbuild.bff";
    const char* copyOfBFF           = "../tmp/Test/Graph/BFFDirtied/fbuild.bff";
    const char* dbFile              = "../tmp/Test/Graph/BFFDirtied/fbuild.fdb";

    EnsureFileDoesNotExist( copyOfBFF );
    EnsureFileDoesNotExist( dbFile );

    // Ensure test output dir exists
    {
        AStackString<> copyOfBFFPath( copyOfBFF );
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

    #if defined( __OSX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of HFS+
    #elif defined( __LINUX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of ext2/ext3/reiserfs and time caching used by used by others
    #endif

    // Modity BFF (make it empty)
    {
        FileStream fs;
        TEST_ASSERT( fs.Open( copyOfBFF, FileStream::WRITE_ONLY ) );
    }

    // Load from dirtied BFF
    {
        FBuild fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Ensure user was informed of reparsing trigger
        TEST_ASSERT( GetRecordedOutput().Find( "has changed (reparsing will occur)" ) );

        // Make sure settings don't "leak" from the original BFF into the new one
        TEST_ASSERT( fBuild.GetSettings()->GetCachePath().IsEmpty() );
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
    MemoryStream ms;
    ms.WriteBuffer( &header, sizeof( header ) );

    // Since we're poking this, we want to know if the layout ever changes somehow
    TEST_ASSERT( ms.GetFileSize() == 4 );
    TEST_ASSERT( ( (const char *)ms.GetDataMutable() )[3] == NodeGraphHeader::NODE_GRAPH_CURRENT_VERSION );

    ( (char *)ms.GetDataMutable() )[3] = ( NodeGraphHeader::NODE_GRAPH_CURRENT_VERSION - 1 );

    const char* oldDB       = "../tmp/Test/Graph/DBVersionChanged/fbuild.fdb";
    const char* emptyBFF    = "../tmp/Test/Graph/DBVersionChanged/fbuild.bff";

    FBuildTestOptions options;
    options.m_ConfigFile = emptyBFF;
    FBuild fBuild( options );

    // cleanup & prep
    {
        AStackString<> oldDBPath( oldDB );
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

//------------------------------------------------------------------------------
