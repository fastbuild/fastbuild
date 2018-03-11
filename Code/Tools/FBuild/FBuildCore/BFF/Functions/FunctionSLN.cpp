// FunctionSLN
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuild
#include "FunctionSLN.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionSLN::FunctionSLN()
: Function( "VSSolution" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSLN::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionSLN::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name;
    if ( GetNameForNode( nodeGraph, funcStartIter, SLNNode::GetReflectionInfoS(), name ) == false )
    {
        return false;
    }
    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }

    SLNNode * slnNode = nodeGraph.CreateSLNNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, slnNode ) )
    {
        return false;
    }

    if ( !slnNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    return ProcessAlias( nodeGraph, funcStartIter, slnNode );
}

//------------------------------------------------------------------------------
