// TestNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "TestNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Process.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
TestNode::TestNode( const AString & testOutput,
					FileNode * testExecutable,
					const AString & arguments,
					const AString & workingDir )
	: FileNode( testOutput, Node::FLAG_NO_DELETE_ON_FAIL ) // keep output on test fail
	, m_Executable( testExecutable )
	, m_Arguments( arguments )
	, m_WorkingDir( workingDir )
{
	ASSERT( testExecutable );
	m_StaticDependencies.Append( Dependency( testExecutable ) );
	m_Type = TEST_NODE;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
TestNode::~TestNode()
{
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TestNode::DoBuild( Job * job )
{
	// If the workingDir is empty, use the current dir for the process
	const char * workingDir = m_WorkingDir.IsEmpty() ? nullptr : m_WorkingDir.Get();

	EmitCompilationMessage( workingDir );

	// spawn the process
	Process p;
	bool spawnOK = p.Spawn( m_Executable->GetName().Get(),
							m_Arguments.Get(),
							workingDir,
							FBuild::Get().GetEnvironmentString() );

	if ( !spawnOK )
	{
		FLOG_ERROR( "Failed to spawn process for '%s'", GetName().Get() );
		return NODE_RESULT_FAILED;
	}

	// capture all of the stdout and stderr
	AutoPtr< char > memOut;
	AutoPtr< char > memErr;
	uint32_t memOutSize = 0;
	uint32_t memErrSize = 0;
	p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

	ASSERT( !p.IsRunning() );
	// Get result
	int result = p.WaitForExit();
	if ( result != 0 )
	{
		// something went wrong, print details
		Node::DumpOutput( job, memOut.Get(), memOutSize );
		Node::DumpOutput( job, memErr.Get(), memErrSize );
	}

	// write the test output (saved for pass or fail)
	FileStream fs;
	if ( fs.Open( GetName().Get(), FileStream::WRITE_ONLY ) == false )
	{
		FLOG_ERROR( "Failed to open test output file '%s'", GetName().Get() );
		return NODE_RESULT_FAILED;
	}
	if ( ( memOut.Get() && ( fs.Write( memOut.Get(), memOutSize ) != memOutSize ) ) ||
		 ( memErr.Get() && ( fs.Write( memErr.Get(), memErrSize ) != memErrSize ) ) )
	{
		FLOG_ERROR( "Failed to write test output file '%s'", GetName().Get() );
		return NODE_RESULT_FAILED;
	}
	fs.Close();

	// did the test fail?
	if ( result != 0 )
	{
		FLOG_ERROR( "Test failed (error %i) '%s'", result, GetName().Get() );
		return NODE_RESULT_FAILED;
	}

	// test passed 
	// we only keep the "last modified" time of the test output for passed tests
	m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
	return NODE_RESULT_OK;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void TestNode::EmitCompilationMessage( const char * workingDir ) const
{
	AStackString<> output;
	output += "Running Test: ";
	output += GetName();
	output += '\n';
	if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
	{
		output += m_Executable->GetName();
		output += ' ';
		output += m_Arguments;
		output += '\n';
		if ( workingDir )
		{
			output += "Working Dir: ";
			output += workingDir;
			output += '\n';
		}
	}
    FLOG_BUILD_DIRECT( output.Get() );
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void TestNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE( m_Executable->GetName() );
	NODE_SAVE( m_Arguments );
	NODE_SAVE( m_WorkingDir );
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * TestNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	fileName );
	NODE_LOAD( AStackString<>,	executable );
	NODE_LOAD( AStackString<>,	arguments );
	NODE_LOAD( AStackString<>,	workingDir );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	Node * execNode = ng.FindNode( executable );
	ASSERT( execNode ); // load/save logic should ensure the src was saved first
	ASSERT( execNode->IsAFile() );

	TestNode * n = ng.CreateTestNode( fileName, 
									  (FileNode *)execNode,
									  arguments,
									  workingDir );
	ASSERT( n );
	return n;
}

//------------------------------------------------------------------------------
