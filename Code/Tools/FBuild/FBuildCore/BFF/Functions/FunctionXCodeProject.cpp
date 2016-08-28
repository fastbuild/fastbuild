// FunctionXCodeProject
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuild
#include "FunctionXCodeProject.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/XCodeProjectNode.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionXCodeProject::FunctionXCodeProject()
: Function( "XCodeProject" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionXCodeProject::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionXCodeProject::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name;
    if ( GetNameForNode( nodeGraph, funcStartIter, XCodeProjectNode::GetReflectionInfoS(), name ) == false )
    {
        return false;
    }

    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }

    auto * xcodeProjNode = nodeGraph.CreateXCodeProjectNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, xcodeProjNode ) )
    {
        return false;
    }

    if ( !xcodeProjNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    // handle alias creation
    return ProcessAlias( nodeGraph, funcStartIter, xcodeProjNode );
}

//------------------------------------------------------------------------------
