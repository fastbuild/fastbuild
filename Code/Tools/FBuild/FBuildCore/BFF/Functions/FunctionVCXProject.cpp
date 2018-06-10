// FunctionVCXProject
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuild
#include "FunctionVCXProject.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionVCXProject::FunctionVCXProject()
: Function( "VCXProject" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionVCXProject::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionVCXProject::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name;
    if ( GetNameForNode( nodeGraph, funcStartIter, VCXProjectNode::GetReflectionInfoS(), name ) == false )
    {
        return false;
    }

    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }

    auto * vcxProjNode = nodeGraph.CreateVCXProjectNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, vcxProjNode ) )
    {
        return false;
    }

    if ( !vcxProjNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    // handle alias creation
    return ProcessAlias( nodeGraph, funcStartIter, vcxProjNode );
}

//------------------------------------------------------------------------------
