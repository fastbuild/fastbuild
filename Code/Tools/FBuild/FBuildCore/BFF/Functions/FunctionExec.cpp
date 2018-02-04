// FunctionTest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionExec.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionExec::FunctionExec()
: Function( "Exec" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionExec::AcceptsHeader() const
{
    return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionExec::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name;
    if ( GetNameForNode( nodeGraph, funcStartIter, ExecNode::GetReflectionInfoS(), name ) == false )
    {
        return false;
    }

    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }
    ExecNode * execNode = nodeGraph.CreateExecNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, execNode ) )
    {
        return false;
    }

    if ( !execNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    // handle alias creation
    return ProcessAlias( nodeGraph, funcStartIter, execNode );
}

//------------------------------------------------------------------------------
