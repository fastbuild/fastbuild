// FunctionVSProjectExternal
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "FunctionVSProjectExternal.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/VSProjectExternalNode.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionVSProjectExternal::FunctionVSProjectExternal()
: Function( "VSProjectExternal" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionVSProjectExternal::AcceptsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionVSProjectExternal::CreateNode() const
{
    return FNEW( VSProjectExternalNode );
}

//------------------------------------------------------------------------------
