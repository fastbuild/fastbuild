// FileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CopyNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CopyNode::CopyNode( const AString & dstFileName,
					FileNode * sourceFile,
					const Dependencies & preBuildDependencies )
: FileNode( dstFileName, Node::FLAG_NONE )
, m_SourceFile( sourceFile )
{
	ASSERT( sourceFile );
	ASSERT( sourceFile->IsAFile() );
	m_StaticDependencies.Append( Dependency( m_SourceFile ) );
	m_Type = Node::COPY_NODE;

	m_PreBuildDependencies = preBuildDependencies;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CopyNode::~CopyNode()
{
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CopyNode::DoBuild( Job * UNUSED( job ) )
{
	EmitCopyMessage();

	// copy the file
	if ( FileIO::FileCopy( m_SourceFile->GetName().Get(), m_Name.Get() ) == false )
	{
		FLOG_ERROR( "Copy failed (error %i) '%s'", Env::GetLastErr(), GetName().Get() );
		return NODE_RESULT_FAILED; // copy failed
	}

	if ( FileIO::SetReadOnly( m_Name.Get(), false ) == false )
	{
		FLOG_ERROR( "Copy read-only flag set failed (error %i) '%s'", Env::GetLastErr(), GetName().Get() );
		return NODE_RESULT_FAILED; // failed to remove read-only
	}

	// update the file's "last modified" time to be equal to the src file
	// TODO:C Use m_SourceFile->GetTimeStamp() when sibling nodes are supported
	// (because we use PreBuildDependencies as a work around, the source time stamp
	// can be taken before the copy is done, so get the current time again here for now)
	//m_Stamp = m_SourceFile->GetStamp();
	m_Stamp = FileIO::GetFileLastWriteTime( m_SourceFile->GetName() );
	ASSERT( m_Stamp );
	if ( FileIO::SetFileLastWriteTime( m_Name, m_Stamp ) == false )
	{
		FLOG_ERROR( "Copy set last write time failed (error %i) '%s'", Env::GetLastErr(), GetName().Get() );
		m_Stamp = 0;
		return NODE_RESULT_FAILED; // failed to set the time
	}

	return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CopyNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	fileName);
	NODE_LOAD( AStackString<>,	sourceFile );
	NODE_LOAD_DEPS( 0,			preBuildDependencies );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Node * srcNode = ng.FindNode( sourceFile );
	ASSERT( srcNode ); // load/save logic should ensure the src was saved first
	ASSERT( srcNode->IsAFile() );
	CopyNode * n = ng.CreateCopyNode( fileName, (FileNode *)srcNode, preBuildDependencies );
	ASSERT( n );
	return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CopyNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE( m_SourceFile->GetName() );
	NODE_SAVE_DEPS( m_PreBuildDependencies );
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void CopyNode::EmitCopyMessage() const
{
	// we combine everything into one string to ensure it is contiguous in
	// the output
	AStackString<> output;
	output += "Copy: ";
	output += m_StaticDependencies[ 0 ].GetNode()->GetName();
	output += " -> ";
	output += GetName();
	output += '\n';
    FLOG_BUILD_DIRECT( output.Get() );
}

//------------------------------------------------------------------------------
