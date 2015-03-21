// CompilerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CompilerNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::CompilerNode( const AString & exe,
						    const Dependencies & extraFiles,
							bool allowDistribution )
	: FileNode( exe, Node::FLAG_NO_DELETE_ON_FAIL )
{
	m_StaticDependencies = extraFiles;
	m_Type = Node::COMPILER_NODE;
	m_AllowDistribution = allowDistribution;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::~CompilerNode()
{
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CompilerNode::DoBuild( Job * job )
{
	// ensure our timestamp is updated (Generate requires this)
	FileNode::DoBuild( job );

	if ( !m_Manifest.Generate( this, m_StaticDependencies ) )
	{
		return Node::NODE_RESULT_FAILED; // Generate will have emitted error
	}

	m_Stamp = m_Manifest.GetTimeStamp();
	return Node::NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CompilerNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>, exeName );
	NODE_LOAD_DEPS( 16,		   staticDeps );
	NODE_LOAD( bool,		   allowDistribution );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	CompilerNode * n = ng.CreateCompilerNode( exeName, staticDeps, allowDistribution );
	n->m_Manifest.Deserialize( stream );
	return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CompilerNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE( m_AllowDistribution );
	m_Manifest.Serialize( stream );
}

//------------------------------------------------------------------------------
