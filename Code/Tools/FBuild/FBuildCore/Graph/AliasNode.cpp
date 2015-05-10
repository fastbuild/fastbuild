// AliasNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "AliasNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileStream.h"

// Reflection
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
REFLECT_BEGIN( AliasNode, Node, MetaNone() )
	REFLECT_ARRAY( m_Targets,	"Targets",			MetaFile() )
REFLECT_END( AliasNode )
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
AliasNode::AliasNode()
: Node( AString::GetEmpty(), Node::ALIAS_NODE, Node::FLAG_TRIVIAL_BUILD )
{
	m_LastBuildTimeMs = 1; // almost no work is done for this node
}
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#ifndef USE_NODE_REFLECTION
AliasNode::AliasNode( const AString & groupName,
					  const Dependencies & targets )
: Node( groupName, Node::ALIAS_NODE, Node::FLAG_TRIVIAL_BUILD )
{
	m_LastBuildTimeMs = 1; // almost no work is done for this node
	m_StaticDependencies = targets;
}
#endif

// Initialize
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
bool AliasNode::Initialize( const BFFIterator & iter, const Function * function )
{
	// TODO:B make this use m_Targets
	Dependencies targets( 32, true );
	if ( !function->GetNodeList( iter, ".Targets", targets, false ) ) // optional
	{
		return false; // GetNodeList will have emitted an error
	}

	m_StaticDependencies = targets;

	return true;
}
#endif

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
#ifdef USE_NODE_REFLECTION
	NODE_LOAD( AStackString<>, name );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	AliasNode * an = ng.CreateAliasNode( name );

	if ( an->Deserialize( stream ) == false )
    {
		return nullptr;
	}
	return an;
#else
	NODE_LOAD( AStackString<>, name );
	NODE_LOAD_DEPS( 0, targets );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	return ng.CreateAliasNode( name, targets );
#endif
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void AliasNode::Save( IOStream & stream ) const
{
#ifdef USE_NODE_REFLECTION
	NODE_SAVE( m_Name );
	Node::Serialize( stream );
#else
	NODE_SAVE( m_Name );
	NODE_SAVE_DEPS( m_StaticDependencies );
#endif
}

//------------------------------------------------------------------------------
