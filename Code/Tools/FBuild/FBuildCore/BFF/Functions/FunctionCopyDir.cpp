// FunctionAlias
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionCopyDir.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionCopyDir::FunctionCopyDir()
: Function( "CopyDir" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCopyDir::AcceptsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCopyDir::Commit( const BFFIterator & funcStartIter ) const
{
	// Get input paths
	Array< AString > inputPaths;
	if ( !GetFolderPaths( funcStartIter, inputPaths, ".SourcePaths", true ) )
	{
		return false; // GetFolderPaths will have emitted an error
	}

	// get the optional params
	AStackString<> pattern;
	bool recurse = true;
	Array< AString > excludePaths;
	if ( !GetString( funcStartIter, pattern, ".SourcePathsPattern" ) ||
		 !GetBool( funcStartIter, recurse, ".SourcePathsRecurse", true ) || // recursive by default
		 !GetStrings( funcStartIter, excludePaths, ".SourceExcludePaths" ) )
	{
		return false; // Get* will have emitted error
	}
	if ( pattern.IsEmpty() )
	{
		pattern = "*";
	}

	// convert input paths to DirectoryListNodes
	Dependencies staticDeps( inputPaths.GetSize() );
	if ( !GetDirectoryListNodeList( funcStartIter, inputPaths, excludePaths, Array< AString >(), recurse, pattern, ".SourcePaths", staticDeps ) )
	{
		return false; // GetDirectoryListNodeList will have emitted an error
	}

	// Get output path
	AStackString<> destPath;
	if ( !GetString( funcStartIter, destPath, ".Dest", true ) )
	{
		return false;
	}
	PathUtils::FixupFolderPath( destPath );

	// Pre-build dependencies
	Dependencies preBuildDeps;
	if ( !GetNodeList( funcStartIter, ".PreBuildDependencies", preBuildDeps, false ) )
	{
		return false; // GetNodeList will have emitted an error
	}

	// sanity check we defined something useful
	if ( staticDeps.IsEmpty() )
	{
		Error::Error_1006_NothingToBuild( funcStartIter, this );
		return false;
	}

	// check node doesn't already exist
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();	
	if ( ng.FindNode( m_AliasForFunction ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
		return false;
	}

	// create our node
	ng.CreateCopyDirNode( m_AliasForFunction, staticDeps, destPath, preBuildDeps );
	return true;
}

//------------------------------------------------------------------------------
