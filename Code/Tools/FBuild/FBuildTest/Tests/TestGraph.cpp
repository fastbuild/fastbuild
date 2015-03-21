// TestGraph.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CSNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
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
	void SingleFileNode() const;
	void SingleFileNodeMissing() const;
	void TestExecNode() const;
	void TestDirectoryListNode() const;
	void TestSerialization() const;
	void TestDeepGraph() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestGraph )
	REGISTER_TEST( EmptyGraph )
	REGISTER_TEST( TestNodeTypes )
	REGISTER_TEST( TestCleanPath )
	REGISTER_TEST( SingleFileNode )
	REGISTER_TEST( SingleFileNodeMissing )
	REGISTER_TEST( TestExecNode )
	REGISTER_TEST( TestDirectoryListNode )
	REGISTER_TEST( TestSerialization )
	REGISTER_TEST( TestDeepGraph )
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
	NodeGraph & ng = fb.GetDependencyGraph();

	FileNode * fn = ng.CreateFileNode( AStackString<>( "file" ) );
	TEST_ASSERT( fn->GetType() == Node::FILE_NODE);
	TEST_ASSERT( FileNode::GetType() == Node::FILE_NODE);

	CompilerNode * cn( nullptr );
	{
		Dependencies extraFiles( 0, false );
		cn = ng.CreateCompilerNode( AStackString<>( "cl.exe" ), extraFiles, false );
		TEST_ASSERT( cn->GetType() == Node::COMPILER_NODE );
		TEST_ASSERT( AStackString<>( "Compiler" ) == cn->GetTypeName() );
	}

	{
		Dependencies empty;
		Node * n = ng.CreateCopyNode( AStackString<>( "aaa" ), fn, empty );
		TEST_ASSERT( n->GetType() == Node::COPY_NODE );
		TEST_ASSERT( CopyNode::GetType() == Node::COPY_NODE );
		TEST_ASSERT( AStackString<>( "Copy" ) == n->GetTypeName() );
	}

	DirectoryListNode * dn = ng.CreateDirectoryListNode( AStackString<>( "path\\|*.cpp|false|" ),
											AStackString<>( "path\\" ),
											AStackString<>( "*.cpp"),
											false,
											Array< AString >() );
	TEST_ASSERT( dn->GetType() == Node::DIRECTORY_LIST_NODE );
	TEST_ASSERT( DirectoryListNode::GetType() == Node::DIRECTORY_LIST_NODE );
	TEST_ASSERT( AStackString<>( "Directory" ) == dn->GetTypeName() );

	{
		Dependencies empty;
		Node * n = ng.CreateExecNode( AStackString<>( "dst" ), fn, fn, AStackString<>( "args" ), AStackString<>( "workingDir" ), 0, empty );
		TEST_ASSERT( n->GetType() == Node::EXEC_NODE );
		TEST_ASSERT( ExecNode::GetType() == Node::EXEC_NODE);
		TEST_ASSERT( AStackString<>( "Exec" ) == n->GetTypeName() );
	}
	{
		Dependencies nodes( 1, false );
		nodes.Append( Dependency( fn ) );
		Node * n = ng.CreateLibraryNode( AStackString<>( "lib" ), nodes, cn,
										 AString::GetEmpty(), AString::GetEmpty(), AString::GetEmpty(),
										 AString::GetEmpty(), AString::GetEmpty(), 0,
										 nullptr, 
										 Dependencies(),
										 Dependencies(),
										 Dependencies(),
										 false, false );
		TEST_ASSERT( n->GetType() == Node::LIBRARY_NODE );
		TEST_ASSERT( LibraryNode::GetType() == Node::LIBRARY_NODE );
		TEST_ASSERT( AStackString<>( "Library" ) == n->GetTypeName() );
	}
	{
		Node * n = ng.CreateObjectNode( AStackString<>( "obj" ), fn, cn, AString::GetEmpty(), AString::GetEmpty(), nullptr, 0, Dependencies(), false, false );
		TEST_ASSERT( n->GetType() == Node::OBJECT_NODE );
		TEST_ASSERT( ObjectNode::GetType() == Node::OBJECT_NODE );
		TEST_ASSERT( AStackString<>( "Object" ) == n->GetTypeName() );
	}
	{
		Dependencies targets( 0, false );
		Node * n = ng.CreateAliasNode( AStackString<>( "alias" ), targets );
		TEST_ASSERT( n->GetType() == Node::ALIAS_NODE );
		TEST_ASSERT( AliasNode::GetType() == Node::ALIAS_NODE );
		TEST_ASSERT( AStackString<>( "Alias" ) == n->GetTypeName() );
	}
	{
		Dependencies libraries( 1, false );
		libraries.Append( Dependency( fn ) );
		Node * n = ng.CreateDLLNode( AStackString<>( "zz.dll" ), libraries, Dependencies(), AString::GetEmpty(), AString::GetEmpty(), 0, Dependencies(), AStackString<>(), nullptr, AString::GetEmpty() );
		TEST_ASSERT( n->GetType() == Node::DLL_NODE );
		TEST_ASSERT( DLLNode::GetType() == Node::DLL_NODE );
		TEST_ASSERT( AStackString<>( "DLL" ) == n->GetTypeName() );
	}
	{
		Dependencies libraries( 1, false );
		libraries.Append( Dependency( fn ) );
		Node * n = ng.CreateExeNode( AStackString<>( "zz.exe" ), libraries, Dependencies(), AString::GetEmpty(), AString::GetEmpty(), 0, Dependencies(), nullptr, AString::GetEmpty() );
		TEST_ASSERT( n->GetType() == Node::EXE_NODE );
		TEST_ASSERT( ExeNode::GetType() == Node::EXE_NODE );
		TEST_ASSERT( AStackString<>( "Exe" ) == n->GetTypeName() );
	}
	{
		Dependencies dNodes( 1, false );
		dNodes.Append( Dependency( dn ) );
		Node * n = ng.CreateUnityNode( AStackString<>( "Unity" ), 
									dNodes,
									Array< AString >(),
									AStackString<>( "C:\\" ),
									AString::GetEmpty(),
									3,
									AString::GetEmpty(),
									Array< AString >(),
									Array< AString >(),
									false,
									0,
									Array< AString >() );
		TEST_ASSERT( n->GetType() == Node::UNITY_NODE);
		TEST_ASSERT( UnityNode::GetType() == Node::UNITY_NODE );
		TEST_ASSERT( AStackString<>( "Unity" ) == n->GetTypeName() );
	}
	{
		Dependencies files( 1, false );
		files.Append( Dependency( fn ) );
		Node * n = ng.CreateCSNode( AStackString<>( "a.cs" ), 
									files,
									AString::GetEmpty(),
									AString::GetEmpty(),
									Dependencies() );
		TEST_ASSERT( n->GetType() == Node::CS_NODE);
		TEST_ASSERT( CSNode::GetType() == Node::CS_NODE );
		TEST_ASSERT( AStackString<>( "C#" ) == n->GetTypeName() );
	}
}

// FileNode
//------------------------------------------------------------------------------
void TestGraph::SingleFileNode() const
{
	FBuild fb;
	NodeGraph & ng = fb.GetDependencyGraph();

	// make sure a node of the name we are going to use doesn't exist
	const AStackString<> testFileName( "SimpleLibrary/library.cpp" );
	TEST_ASSERT( ng.FindNode( testFileName ) == false );

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
	NodeGraph & ng = fb.GetDependencyGraph();

	// make a node for a file that does not exist
	const AStackString<> testFileName( "ThisFileDoesNotExist.cpp" );
	FileNode * node = ng.CreateFileNode( testFileName );

	// make sure build still passes
	// a missing file is not an error.  it would need to be required by something
	// (like an objectNode which would handle the failure itself)
	TEST_ASSERT( fb.Build( node ) == true );
}

// TestExecNode
//------------------------------------------------------------------------------
void TestGraph::TestExecNode() const
{
	FBuild fb;
	NodeGraph & ng = fb.GetDependencyGraph();

	const AString & workingDir( AString::GetEmpty() ); // empty = use the "current" dir

	// just do a file copy
	const AStackString<> exeName( "c:\\Windows\\System32\\robocopy.exe" ); // file copy
	const AStackString<> srcFile( "Data/TestGraph/library.cpp" );
	const AStackString<> dstFile( "..\\..\\..\\..\\ftmp\\Test\\Graph\\TestExecNode\\library.cpp" );
	AStackString<> exeArgs;
	exeArgs.Format( "Data\\TestGraph ..\\..\\..\\..\\ftmp\\Test\\Graph\\TestExecNode library.cpp /COPY:D" );

	// make sure everything on disc is as we need/expect
	if ( FileIO::FileExists( dstFile.Get() ) )
	{
		TEST_ASSERT( FileIO::SetReadOnly( dstFile.Get(), false ) );
		TEST_ASSERT( FileIO::FileDelete( dstFile.Get() ) );
	}
	TEST_ASSERT( FileIO::FileExists( dstFile.Get() ) == false );

	// setup graph
	FileNode * srcNode = ng.CreateFileNode( srcFile );
	FileNode * exe = ng.CreateFileNode( exeName, false ); // false == don't clean the path
	Dependencies empty;
	ExecNode * execNode = ng.CreateExecNode( dstFile, srcNode, exe, exeArgs, workingDir, 1, empty );

	// build and verify
	TEST_ASSERT( fb.Build( execNode ) );

	// check file was actually copied
	TEST_ASSERT( FileIO::FileExists( dstFile.Get() ) );
	FileStream f;
	f.Open( srcFile.Get() );
	uint64_t srcSize = f.GetFileSize();
	f.Close();
	f.Open( dstFile.Get() );
	uint64_t dstSize = f.GetFileSize();
	f.Close();

	TEST_ASSERT( srcSize == dstSize );
}

// TestDirectoryListNode
//------------------------------------------------------------------------------
void TestGraph::TestDirectoryListNode() const
{
	FBuild fb;
	NodeGraph & ng = fb.GetDependencyGraph();

	// make sure a node of the name we are going to use doesn't exist
	const AStackString<> testFolder( "Data\\TestGraph\\" );

	// create the node, and make sure we can access it by name
	const AStackString<> name( "Data\\TestGraph\\|library.*|true|" );
	DirectoryListNode * node = ng.CreateDirectoryListNode( name,
														   testFolder,
														   AStackString<>( "library.*" ),
														   true,
														   Array< AString >() );
	TEST_ASSERT( ng.FindNode( name ) == node );

	TEST_ASSERT( fb.Build( node ) );

	// make sure we got the expected results
	TEST_ASSERT( node->GetFiles().GetSize() == 2 );
	TEST_ASSERT( node->GetFiles()[ 0 ].m_Name.EndsWith( "Data\\TestGraph\\library.cpp" ) );
	TEST_ASSERT( node->GetFiles()[ 1 ].m_Name.EndsWith( "Data\\TestGraph\\library.o" ) );
}

// TestSerialization
//------------------------------------------------------------------------------
void TestGraph::TestSerialization() const
{
	AStackString<> codeDir;
	GetCodeDir( codeDir );

	const char * dbFile1	= "../ftmp/Test/Graph/fbuild.db";
	const char * dbFile2	= "../ftmp/Test/Graph/fbuild.db.2"; 

	// clean up anything left over from previous runs
	FileIO::FileDelete( dbFile1 );
	FileIO::FileDelete( dbFile2 );
	TEST_ASSERT( FileIO::FileExists( dbFile1 ) == false );
	TEST_ASSERT( FileIO::FileExists( dbFile2 ) == false );

	// load the config file and save the resulting db
	{
		FBuildOptions options;
		options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";
		options.SetWorkingDir( codeDir );
		FBuild fBuild( options );
		TEST_ASSERT( fBuild.Initialize() );
		TEST_ASSERT( fBuild.SaveDependencyGraph( dbFile1 ) );
		TEST_ASSERT( FileIO::FileExists( dbFile1 ) );
	}

	// load the config, this time from the db, and save it again
	{
		FBuildOptions options;
		options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestBuildFBuild/fbuild.bff";
		options.SetWorkingDir( codeDir );
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
	fo.SetWorkingDir( AStackString<>( "C:\\Windows\\System32" ) );

	FBuild f( fo );

	#define CHECK(a, b) \
	{ \
		AStackString<> cleaned; \
		NodeGraph::CleanPath( AStackString<>( a ), cleaned ); \
		TEST_ASSERT( cleaned == b ); \
	}

	//   "\..\"
	CHECK( "file.dat",				"C:\\Windows\\System32\\file.dat" )
	CHECK( "..\\file.dat",			"C:\\Windows\\file.dat" )
	CHECK( "..\\..\\file.dat",		"C:\\file.dat" )
	CHECK( "..\\..\\..\\file.dat",	"C:\\file.dat" )

	//   "/../"
	CHECK( "file.dat",				"C:\\Windows\\System32\\file.dat" )
	CHECK( "../file.dat",			"C:\\Windows\\file.dat" )
	CHECK( "../../file.dat",		"C:\\file.dat" )
	CHECK( "../../../file.dat",		"C:\\file.dat" )

	//   "\.\"
	CHECK( ".\\file.dat",			"C:\\Windows\\System32\\file.dat" )
	CHECK( "folder\\.\\file.dat",	"C:\\Windows\\System32\\folder\\file.dat" )
	CHECK( ".\\.\\.\\file.dat",		"C:\\Windows\\System32\\file.dat" )

	//   "/./"
	CHECK( "./file.dat",			"C:\\Windows\\System32\\file.dat" )
	CHECK( "folder/./file.dat",		"C:\\Windows\\System32\\folder\\file.dat" )
	CHECK( "./././file.dat",		"C:\\Windows\\System32\\file.dat" )

	//   full path '\'
	CHECK( "C:\\Windows\\System32\\file.dat",				"C:\\Windows\\System32\\file.dat" )
	CHECK( "C:\\Windows\\System32\\..\\file.dat",			"C:\\Windows\\file.dat" )
	CHECK( "C:\\Windows\\System32\\..\\..\\file.dat",		"C:\\file.dat" )
	CHECK( "C:\\Windows\\System32\\..\\..\\..\\file.dat",	"C:\\file.dat" )

	//   full path '/'
	CHECK( "C:/Windows/System32/file.dat",				"C:\\Windows\\System32\\file.dat" )
	CHECK( "C:/Windows/System32/../file.dat",			"C:\\Windows\\file.dat" )
	CHECK( "C:/Windows/System32/../../file.dat",		"C:\\file.dat" )
	CHECK( "C:/Windows/System32/../../../file.dat",		"C:\\file.dat" )

	// files with . in them
	CHECK( ".file.dat",		"C:\\Windows\\System32\\.file.dat" )
	CHECK( ".file",			"C:\\Windows\\System32\\.file" )
	CHECK( "subdir\\.file",	"C:\\Windows\\System32\\subdir\\.file" )

	// multiple slash removal
	CHECK( "subdir\\\\.file",		"C:\\Windows\\System32\\subdir\\.file" )
	CHECK( "subdir//.file",			"C:\\Windows\\System32\\subdir\\.file" )
	CHECK( "subdir//.//.file",		"C:\\Windows\\System32\\subdir\\.file" )
	CHECK( "subdir\\\\.\\\\.file",	"C:\\Windows\\System32\\subdir\\.file" )
	CHECK( "subdir\\\\..\\\\.file",	"C:\\Windows\\System32\\.file" )
	CHECK( "subdir//..//.file",		"C:\\Windows\\System32\\.file" )

	// edge cases/regressions
	CHECK( "\\folder\\file",        "C:\\Windows\\System32\\folder\\file" )

	#undef CHECK
}

// TestDeepGraph
//------------------------------------------------------------------------------
void TestGraph::TestDeepGraph() const
{
	FBuildOptions options;
	options.m_ConfigFile = "Data/TestGraph/DeepGraph.bff";
	options.m_UseCacheRead = true;
	options.m_UseCacheWrite = true;

	const char * dbFile1 = "../../../../ftmp/Test/Graph/DeepGraph.fdb";

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
		options.m_ShowSummary = true; // required to generate stats for node count checks
		FBuild fBuild( options );
		TEST_ASSERT( fBuild.Initialize( dbFile1 ) );
		TEST_ASSERT( fBuild.Build( AStackString<>( "all" ) ) );
		CheckStatsNode ( 30,		0,		Node::OBJECT_NODE );

		// make sure walking the graph wasn't slow (should be a good deal less 
		// than 100ms, but allow for a lot of slack on the test machine)
		TEST_ASSERT( t.GetElapsed() < 2.0f );
	}
}

//------------------------------------------------------------------------------
