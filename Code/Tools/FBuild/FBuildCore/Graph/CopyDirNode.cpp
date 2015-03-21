// FileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CopyDirNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

//#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CopyDirNode::CopyDirNode( const AString & name,
						  Dependencies & staticDeps,
						  const AString & destPath,
						  const Dependencies & preBuildDeps )
: Node( name, Node::COPY_DIR_NODE, Node::FLAG_NONE )
, m_DestPath( destPath )
{
	m_StaticDependencies.Append( staticDeps );
	m_PreBuildDependencies = preBuildDeps;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CopyDirNode::~CopyDirNode()
{
}

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool CopyDirNode::IsAFile() const
{
	return false;
}

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool CopyDirNode::DoDynamicDependencies( bool forceClean )
{
	(void)forceClean; // dynamic deps are always re-added here, so this is meaningless

	ASSERT( !m_StaticDependencies.IsEmpty() );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();

	// Iterate all the DirectoryListNodes
	const Dependency * const depEnd = m_StaticDependencies.End();
	for ( const Dependency * dep = m_StaticDependencies.Begin();
		  dep != depEnd;
		  ++dep )
	{
		// Grab the files
		DirectoryListNode * dln = dep->GetNode()->CastTo< DirectoryListNode >();
		const Array< FileIO::FileInfo > & files = dln->GetFiles();
		const FileIO::FileInfo * const fEnd = files.End();
		for ( const FileIO::FileInfo * fIt = files.Begin();
			  fIt != fEnd;
			  ++fIt )
		{
			// Create a CopyNode for each dynamically discovered file

			// source file (full path)
			const AString & srcFile = fIt->m_Name;

			// source file (relative to base path)
			const AStackString<> srcFileRel( srcFile.Get() + dln->GetPath().GetLength() );

			// source file (as a node)
			Node * srcFileNode = ng.FindNode( srcFile );
			if ( srcFileNode == nullptr )
			{
				srcFileNode = ng.CreateFileNode( srcFile );
			}
			else if ( srcFileNode->IsAFile() == false )
			{
				FLOG_ERROR( "CopyDir() Node '%s' is not a FileNode (type: %s)", srcFile.Get(), srcFileNode->GetTypeName() );
				return false;
			}

			// generate dest file name
			const AStackString<> dstFile( m_DestPath );
			(AString &)dstFile += (AString &)srcFileRel;

			// make sure dest doesn't already exist
			Node * n = ng.FindNode( dstFile );
			if ( n == nullptr )
			{
				n = ng.CreateCopyNode( dstFile, srcFileNode, m_PreBuildDependencies ); // inherit PreBuildDependencies
			}
			else if ( n->GetType() != Node::COPY_NODE )
			{
				FLOG_ERROR( "Node '%s' is not a CopyNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
				return false;
			}
			else
			{
				CopyNode * cn = n->CastTo< CopyNode >();
				if ( srcFileNode != cn->GetSourceNode() )
				{
					FLOG_ERROR( "Conflicting objects found during CopyDir:\n"
								" File A: %s\n"
								" File B: %s\n"
								" Both copy to: %s\n",
								srcFile.Get(),
								cn->GetSourceNode()->GetName().Get(),
								dstFile.Get() );
					return false;
				}
			}

			m_DynamicDependencies.Append( Dependency( n ) );
		}
	}
	return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CopyDirNode::DoBuild( Job * UNUSED( job ) )
{
	// consider ourselves to be as recent as the newest file
	uint64_t timeStamp = 0;
	const Dependency * const end = m_DynamicDependencies.End();
	for ( const Dependency * it = m_DynamicDependencies.Begin(); it != end; ++it )
	{
		CopyNode * cn = it->GetNode()->CastTo< CopyNode >();
		timeStamp = Math::Max< uint64_t >( timeStamp, cn->GetStamp() );
	}
	m_Stamp = timeStamp;

	return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CopyDirNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>,	name);
	NODE_LOAD_DEPS( 4,			staticDeps );
	NODE_LOAD( AStackString<>,  destPath );
	NODE_LOAD_DEPS( 0,			preBuildDeps );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	CopyDirNode * n = ng.CreateCopyDirNode( name, staticDeps, destPath, preBuildDeps );
	ASSERT( n );
	return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CopyDirNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE( m_DestPath );
	NODE_SAVE_DEPS( m_PreBuildDependencies );
}

//------------------------------------------------------------------------------
