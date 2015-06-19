// ExecNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ExecNode.h"

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
ExecNode::ExecNode( const AString & dstFileName,
					    FileNode * sourceFile,
						FileNode * executable,
						const AString & arguments,
						const AString & workingDir,
						int32_t expectedReturnCode,
						const Dependencies & preBuildDependencies )
: FileNode( dstFileName, Node::FLAG_NONE )
, m_SourceFile( sourceFile )
, m_Executable( executable )
, m_Arguments( arguments )
, m_WorkingDir( workingDir )
, m_ExpectedReturnCode( expectedReturnCode )
{
	ASSERT( sourceFile );
	ASSERT( executable );
	m_StaticDependencies.SetCapacity( 2 );
	m_StaticDependencies.Append( Dependency( sourceFile ) );
	m_StaticDependencies.Append( Dependency( executable ) );
	m_Type = EXEC_NODE;

	m_PreBuildDependencies = preBuildDependencies;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ExecNode::~ExecNode()
{
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ExecNode::DoBuild( Job * job )
{
	// If the workingDir is empty, use the current dir for the process
	const char * workingDir = m_WorkingDir.IsEmpty() ? nullptr : m_WorkingDir.Get();

	AStackString<> fullArgs( m_Arguments );
	fullArgs.Replace( "%1", m_SourceFile->GetName().Get() );
	fullArgs.Replace( "%2", GetName().Get() );

	EmitCompilationMessage( fullArgs );

	// spawn the process
	Process p;
	bool spawnOK = p.Spawn( m_Executable->GetName().Get(),
							fullArgs.Get(),
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

	// did the executable fail?
	if ( result != m_ExpectedReturnCode )
	{
		// something went wrong, print details
		Node::DumpOutput( job, memOut.Get(), memOutSize );
		Node::DumpOutput( job, memErr.Get(), memErrSize );

		FLOG_ERROR( "Execution failed (error %i) '%s'", result, GetName().Get() );
		return NODE_RESULT_FAILED;
	}

	// update the file's "last modified" time
	m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
	return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * ExecNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	fileName );
	NODE_LOAD( AStackString<>,	sourceFile );
	NODE_LOAD( AStackString<>,	executable );
	NODE_LOAD( AStackString<>,	arguments );
	NODE_LOAD( AStackString<>,	workingDir );
	NODE_LOAD( int32_t,			expectedReturnCode );
	NODE_LOAD_DEPS( 0,			preBuildDependencies );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * srcNode = ng.FindNode( sourceFile );
	ASSERT( srcNode ); // load/save logic should ensure the src was saved first
	ASSERT( srcNode->IsAFile() );
	Node * execNode = ng.FindNode( executable );
	ASSERT( execNode ); // load/save logic should ensure the src was saved first
	ASSERT( execNode->IsAFile() );
	ExecNode * n = ng.CreateExecNode( fileName, 
								  (FileNode *)srcNode,
								  (FileNode *)execNode,
								  arguments,
								  workingDir,
								  expectedReturnCode,
								  preBuildDependencies );
	ASSERT( n );

	return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void ExecNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE( m_SourceFile->GetName() );
	NODE_SAVE( m_Executable->GetName() );
	NODE_SAVE( m_Arguments );
	NODE_SAVE( m_WorkingDir );
	NODE_SAVE( m_ExpectedReturnCode );
	NODE_SAVE_DEPS( m_PreBuildDependencies );
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void ExecNode::EmitCompilationMessage( const AString & args ) const
{
	// basic info
	AStackString< 2048 > output;
	output += "Run: ";
	output += GetName();
	output += '\n';

	// verbose mode
	if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
	{
		AStackString< 1024 > verboseOutput;
		verboseOutput.Format( "%s %s\nWorkingDir: %s\nExpectedReturnCode: %i\n", 
							  m_Executable->GetName().Get(), 
							  args.Get(),
							  m_WorkingDir.Get(),
							  m_ExpectedReturnCode );
		output += verboseOutput;
	}

	// output all at once for contiguousness
    FLOG_BUILD_DIRECT( output.Get() );
}

//------------------------------------------------------------------------------
