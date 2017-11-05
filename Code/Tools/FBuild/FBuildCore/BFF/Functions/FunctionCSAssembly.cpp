// FunctionCSAssembly
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionCSAssembly.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/CSNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionCSAssembly::FunctionCSAssembly()
: Function( "CSAssembly" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCSAssembly::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCSAssembly::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name;
    if ( GetNameForNode( nodeGraph, funcStartIter, CSNode::GetReflectionInfoS(), name ) == false )
    {
        return false;
    }

    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }
    CSNode * csNode = nodeGraph.CreateCSNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, csNode ) )
    {
        return false;
    }

    if ( !csNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    // handle alias creation
    return ProcessAlias( nodeGraph, funcStartIter, csNode );
}

//------------------------------------------------------------------------------
