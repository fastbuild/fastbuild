// FunctionVSProjectExternal
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionVSProjectExternal.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Graph/VSProjectExternalNode.h"

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
