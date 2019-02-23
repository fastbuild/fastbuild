// FunctionVSSolution
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "FunctionVSSolution.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionVSSolution::FunctionVSSolution()
: Function( "VSSolution" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionVSSolution::AcceptsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionVSSolution::CreateNode() const
{
    return FNEW( SLNNode );
}

//------------------------------------------------------------------------------
