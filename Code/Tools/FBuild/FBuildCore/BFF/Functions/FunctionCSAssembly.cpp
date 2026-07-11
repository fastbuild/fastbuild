// FunctionCSAssembly
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionCSAssembly.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/CSNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

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
