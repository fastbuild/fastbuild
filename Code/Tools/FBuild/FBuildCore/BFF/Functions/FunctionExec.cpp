// FunctionTest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
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

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionExec::CreateNode() const
{
    return FNEW( ExecNode );
}

//------------------------------------------------------------------------------
