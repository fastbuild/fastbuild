// CompilerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CompilerNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/Strings/AStackString.h"


// Reflection
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
REFLECT_BEGIN( CompilerNode, Node, MetaName( "Executable" ) + MetaFile() )
	REFLECT_ARRAY( m_ExtraFiles,	"ExtraFiles",			MetaOptional() + MetaFile() )
    REFLECT( m_AllowDistribution,   "AllowDistribution",    MetaOptional() )
REFLECT_END( CompilerNode )
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
CompilerNode::CompilerNode()
	: FileNode( AString::GetEmpty(), Node::FLAG_NO_DELETE_ON_FAIL )
    , m_AllowDistribution( true )
{
	m_Type = Node::COMPILER_NODE;
}
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
#ifndef USE_NODE_REFLECTION
CompilerNode::CompilerNode( const AString & exe,
						    const Dependencies & extraFiles,
							bool allowDistribution )
	: FileNode( exe, Node::FLAG_NO_DELETE_ON_FAIL )
{
	m_StaticDependencies = extraFiles;
	m_Type = Node::COMPILER_NODE;
	m_AllowDistribution = allowDistribution;
}
#endif

// Initialize
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
bool CompilerNode::Initialize( const BFFIterator & iter, const Function * function )
{
	// TODO:B make this use m_ExtraFiles
	Dependencies extraFiles( 32, true );
	if ( !function->GetNodeList( iter, ".ExtraFiles", extraFiles, false ) ) // optional
	{
		return false; // GetNodeList will have emitted an error
	}

	m_StaticDependencies = extraFiles;

	return true;
}
#endif

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
#ifdef USE_NODE_REFLECTION
	NODE_LOAD( AStackString<>, name );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	CompilerNode * cn = ng.CreateCompilerNode( name );

	if ( cn->Deserialize( stream ) == false )
	{
		return nullptr;
	}
	cn->m_Manifest.Deserialize( stream, false ); // false == not remote
	return cn;
#else
	NODE_LOAD( AStackString<>, exeName );
	NODE_LOAD_DEPS( 16,		   staticDeps );
	NODE_LOAD( bool,		   allowDistribution );

	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	CompilerNode * n = ng.CreateCompilerNode( exeName, staticDeps, allowDistribution );
	n->m_Manifest.Deserialize( stream, false ); // false == not remote
	return n;
#endif
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CompilerNode::Save( IOStream & stream ) const
{
#ifdef USE_NODE_REFLECTION
	NODE_SAVE( m_Name );
	Node::Serialize( stream );
	m_Manifest.Serialize( stream );
#else
	NODE_SAVE( m_Name );
	NODE_SAVE_DEPS( m_StaticDependencies );
	NODE_SAVE( m_AllowDistribution );
	m_Manifest.Serialize( stream );
#endif
}

//------------------------------------------------------------------------------
