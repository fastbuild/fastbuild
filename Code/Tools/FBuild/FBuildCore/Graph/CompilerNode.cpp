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
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"


// Reflection
//------------------------------------------------------------------------------
REFLECT_BEGIN( CompilerNode, Node, MetaName( "Executable" ) + MetaFile() )
	REFLECT_ARRAY( m_ExtraFiles,	"ExtraFiles",			MetaOptional() + MetaFile() )
    REFLECT( m_AllowDistribution,   "AllowDistribution",    MetaOptional() )
	REFLECT( m_VS2012EnumBugFix,	"VS2012EnumBugFix",		MetaOptional() )
REFLECT_END( CompilerNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::CompilerNode()
	: FileNode( AString::GetEmpty(), Node::FLAG_NO_DELETE_ON_FAIL )
    , m_AllowDistribution( true )
	, m_VS2012EnumBugFix( false )
{
	m_Type = Node::COMPILER_NODE;
}

// Initialize
//------------------------------------------------------------------------------
bool CompilerNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
	// TODO:B make this use m_ExtraFiles
	Dependencies extraFiles( 32, true );
	if ( !function->GetNodeList( nodeGraph, iter, ".ExtraFiles", extraFiles, false ) ) // optional
	{
		return false; // GetNodeList will have emitted an error
	}

	// Check for conflicting files
	AStackString<> relPathExe;
	ToolManifest::GetRelativePath( m_Name, m_Name, relPathExe );

	const size_t numExtraFiles = extraFiles.GetSize();
	for ( size_t i=0; i<numExtraFiles; ++i )
	{
		AStackString<> relPathA;
		ToolManifest::GetRelativePath( m_Name, extraFiles[ i ].GetNode()->GetName(), relPathA );

		// Conflicts with Exe?
		if ( PathUtils::ArePathsEqual( relPathA, relPathExe ) )
		{
			Error::Error_1100_AlreadyDefined( iter, function, relPathA );
			return false;
		}

		// Conflicts with another file?
		for ( size_t j=(i+1); j<numExtraFiles; ++j )
		{
			AStackString<> relPathB;
			ToolManifest::GetRelativePath( m_Name, extraFiles[ j ].GetNode()->GetName(), relPathB );

			if ( PathUtils::ArePathsEqual( relPathA, relPathB ) )
			{
				Error::Error_1100_AlreadyDefined( iter, function, relPathA );
				return false;
			}
		}
	}

	m_StaticDependencies = extraFiles;

	return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::~CompilerNode()
{
}

// DetermineNeedToBuild
//------------------------------------------------------------------------------
bool CompilerNode::DetermineNeedToBuild( bool forceClean ) const
{
	if ( forceClean )
	{
		return true;
	}

	// Building for the first time?
	if ( m_Stamp == 0 )
	{
		return true;
	}

	// check primary file
	const uint64_t fileTime = FileIO::GetFileLastWriteTime( m_Name );
	if ( fileTime > m_Stamp )
	{
		return true;
	}

	// check additional files
	for ( const auto & dep : m_StaticDependencies )
	{
		if ( dep.GetNode()->GetStamp() > m_Stamp )
		{
			return true;
		}
	}

	return false;
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

	m_Stamp = Math::Max( m_Stamp, m_Manifest.GetTimeStamp() );
	return Node::NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CompilerNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
	NODE_LOAD( AStackString<>, name );

	CompilerNode * cn = nodeGraph.CreateCompilerNode( name );

	if ( cn->Deserialize( nodeGraph, stream ) == false )
	{
		return nullptr;
	}
	cn->m_Manifest.Deserialize( stream, false ); // false == not remote
	return cn;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CompilerNode::Save( IOStream & stream ) const
{
	NODE_SAVE( m_Name );
	Node::Serialize( stream );
	m_Manifest.Serialize( stream );
}

//------------------------------------------------------------------------------
