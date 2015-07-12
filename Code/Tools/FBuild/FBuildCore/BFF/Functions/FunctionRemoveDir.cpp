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
/*virtual*/ bool FunctionRemoveDir::Commit( const BFFIterator & funcStartIter ) const
{
    // Get input paths
    Array< AString > inputPaths;
    if ( !GetFolderPaths( funcStartIter, inputPaths, ".Paths", true ) )
    {
        return false; // GetFolderPaths will have emitted an error
    }

    // get the optional params
    AStackString<> pattern;
    bool recurse = true;
    Array< AString > excludePaths;
    if ( !GetString( funcStartIter, pattern, ".PathsPattern" ) ||
         !GetBool( funcStartIter, recurse, ".PathsRecurse", true ) || // recursive by default
         !GetStrings( funcStartIter, excludePaths, ".ExcludePaths" ) )
    {
        return false; // Get* will have emitted error
    }
    if ( pattern.IsEmpty() )
    {
        pattern = "*";
    }

    // convert input paths to DirectoryListNodes
    Dependencies staticDeps( inputPaths.GetSize() );
    if ( !GetDirectoryListNodeList( funcStartIter, inputPaths, excludePaths, Array< AString >(), recurse, pattern, ".Paths", staticDeps ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    // Pre-build dependencies
    Dependencies preBuildDeps;
    bool allowCopyDirNodes = true;
    bool allowUnityNodes = false;
    bool allowRemoveDirNodes = true;
    if ( !GetNodeList( funcStartIter, ".PreBuildDependencies", preBuildDeps, false, allowCopyDirNodes, allowUnityNodes, allowRemoveDirNodes ) )
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
    ng.CreateRemoveDirNode( m_AliasForFunction, staticDeps, preBuildDeps );
    return true;
}

//------------------------------------------------------------------------------
