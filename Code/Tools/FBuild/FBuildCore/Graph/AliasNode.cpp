// AliasNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "AliasNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileStream.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
AliasNode::AliasNode( const AString & groupName,
					  const Dependencies & targets )
: Node( groupName, Node::ALIAS_NODE, Node::FLAG_TRIVIAL_BUILD )
{
	m_LastBuildTimeMs = 1; // almost no work is done for this node
	m_StaticDependencies = targets;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
AliasNode::~AliasNode()
{
}

// DetermineNeedToBuild
//------------------------------------------------------------------------------
/*virtual*/ bool AliasNode::DetermineNeedToBuild( bool forceClean ) const
{
	(void)forceClean;
	return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult AliasNode::DoBuild( Job * UNUSED( job ) )
{
	const Dependencies::Iter end = m_StaticDependencies.End();
	for ( Dependencies::Iter it = m_StaticDependencies.Begin();
		  it != end;
		  ++it )
	{
		// If any nodes are file nodes ...
		const Node * n = it->GetNode();
		if ( n->GetType() == Node::FILE_NODE )
		{
			// ... and the file is missing ...
			if ( n->GetStamp() == 0 )
			{
				// ... the build should fail
				FLOG_ERROR( "Alias: %s\nFailed due to missing file: %s\n", GetName().Get(), n->GetName().Get() );
				return Node::NODE_RESULT_FAILED;
			}
		}
	}
	return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * AliasNode::Load( IOStream & stream )
{
	NODE_LOAD( AStackString<>, name );
	NODE_LOAD_DEPS( 0, targets );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	return ng.CreateAliasNode( name, targets );
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void AliasNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	NODE_SAVE_DEPS( m_StaticDependencies );
}

//------------------------------------------------------------------------------
