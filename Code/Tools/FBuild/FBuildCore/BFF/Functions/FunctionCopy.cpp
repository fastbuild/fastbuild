// FunctionAlias
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionCopy.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionCopy::FunctionCopy()
: Function( "Copy" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCopy::AcceptsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCopy::Commit( const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	Array< AString > sources( 16, true );
	const BFFVariable * dstFileV;
	if ( !GetStrings( funcStartIter, sources, ".Source", true ) ||
		 !GetString( funcStartIter, dstFileV, ".Dest", true ) )
	{
		return false; // GetString will have emitted errors
	}

	// check sources are not paths
	{
		const AString * const end = sources.End();
		for ( const AString * it = sources.Begin(); it != end; ++it )
		{
			const AString & srcFile( *it );

			// source must be a file, not a  path
			if ( PathUtils::IsFolderPath( srcFile ) )
			{
				Error::Error_1105_PathNotAllowed( funcStartIter, this, ".Source", srcFile );
				return false;
			}
		}
	}

	// Pre-build dependencies
	Dependencies preBuildDependencies;
	if ( !GetNodeList( funcStartIter, ".PreBuildDependencies", preBuildDependencies, false ) )
	{
		return false; // GetNodeList will have emitted an error
	}

	// get source node
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	Array< Node * > srcNodes;
	{
		const AString * const end = sources.End();
		for ( const AString * it = sources.Begin(); it != end; ++it )
		{

			Node * srcNode = ng.FindNode( *it );
			if ( srcNode )
			{
				if ( GetSourceNodes( funcStartIter, srcNode, srcNodes ) == false )
				{
					return false;
				}
			}
			else
			{
				// source file not defined by use - assume an external file
				srcNodes.Append( ng.CreateFileNode( *it ) );
			}
		}
	}

	if ( srcNodes.IsEmpty() )
	{
		Error::Error_1006_NothingToBuild( funcStartIter, this );
		return false;
	}

	AStackString<> dstFile;
	NodeGraph::CleanPath( dstFileV->GetString(), dstFile );

	// make all the nodes for copies
	Dependencies copyNodes( srcNodes.GetSize(), false );
	Node * const * end = srcNodes.End();
	for ( Node ** it = srcNodes.Begin(); it != end; ++it )
	{
		AStackString<> dst( dstFile );

		// dest can be a file OR a path.  If it's a path, use the source filename part
		if ( PathUtils::IsFolderPath( dstFile ) )
		{
			// find filename part of source
			const AString & srcName = ( *it )->GetName();
			const char * lastSlash = srcName.FindLast( NATIVE_SLASH );
			dst += lastSlash ? ( lastSlash + 1 )	// append filename part if found
								 : srcName.Get();	// otherwise append whole thing
		}

		// check node doesn't already exist
		if ( ng.FindNode( dst ) )
		{
			// TODO:C could have a specific error for multiple sources with only 1 output
			// to differentiate from two rules creating the same dst target
			Error::Error_1100_AlreadyDefined( funcStartIter, this, dst );
			return false;
		}

		// create our node
		Node * copyNode = ng.CreateCopyNode( dst, *it, preBuildDependencies );
		copyNodes.Append( Dependency( copyNode ) );
	}

	// handle alias creation
	return ProcessAlias( funcStartIter, copyNodes );
}

// GetSourceNodes
//------------------------------------------------------------------------------
bool FunctionCopy::GetSourceNodes( const BFFIterator & iter, Node * node, Array< Node * > & nodes ) const
{
	if ( node->GetType() == Node::ALIAS_NODE )
	{
		// resolve aliases to real nodes
		AliasNode * aliasNode = node->CastTo< AliasNode >();
		const Dependencies & aliasedNodes = aliasNode->GetAliasedNodes();
		const Dependency * const end = aliasedNodes.End();
		for ( const Dependency * it = aliasedNodes.Begin(); it != end; ++it )
		{
			if ( !GetSourceNodes( iter, it->GetNode(), nodes ) )
			{
				return false;
			}
		}
		return true;
	}
	else if ( node->IsAFile() )
	{
		// anything that results in a file is ok
		nodes.Append( node );
		return true;
	}

	// something we don't know how to handle
	Error::Error_1005_UnsupportedNodeType( iter, this, ".Source", node->GetName(), node->GetType() );
	return false;
}

//------------------------------------------------------------------------------
