// FunctionCSAssembly
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
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

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionCSAssembly::CreateNode() const
{
    return FNEW( CSNode );
}

//------------------------------------------------------------------------------
