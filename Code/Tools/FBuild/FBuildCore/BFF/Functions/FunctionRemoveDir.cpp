// FunctionRemoveDir
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionRemoveDir.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionRemoveDir::FunctionRemoveDir()
: Function( "RemoveDir" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionRemoveDir::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionRemoveDir::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    // Get input paths
    Array< AString > inputPaths;
    if ( !GetFolderPaths( funcStartIter, inputPaths, ".RemovePaths", true ) )
    {
        return false; // GetFolderPaths will have emitted an error
    }

    // get the optional params
    Array< AString > patterns;
    bool recurse = true;
    Array< AString > excludePaths;
    if ( !GetStrings( funcStartIter, patterns, ".RemovePatterns" ) ||
         !GetBool( funcStartIter, recurse, ".RemovePathsRecurse", true ) || // recursive by default
         !GetStrings( funcStartIter, excludePaths, ".RemoveExcludePaths" ) )
    {
        return false; // Get* will have emitted error
    }
    if ( patterns.IsEmpty() )
    {
        patterns.Append( AStackString<>( "*" ) );
    }

    // convert input paths to DirectoryListNodes
    Dependencies staticDeps( inputPaths.GetSize() );
    if ( !GetDirectoryListNodeList( nodeGraph, funcStartIter, inputPaths, excludePaths, Array< AString >(), recurse, patterns.IsEmpty() ? nullptr : &patterns, ".RemovePaths", staticDeps ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    // Pre-build dependencies
    Dependencies preBuildDeps;
    bool allowCopyDirNodes = true;
    bool allowUnityNodes = false;
    bool allowRemoveDirNodes = true;
    if ( !GetNodeList( nodeGraph, funcStartIter, ".PreBuildDependencies", preBuildDeps, false, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes ) )
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
    if ( nodeGraph.FindNode( m_AliasForFunction ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, m_AliasForFunction );
        return false;
    }

    // create our node
    nodeGraph.CreateRemoveDirNode( m_AliasForFunction, staticDeps, preBuildDeps );
    return true;
}

//------------------------------------------------------------------------------
