// FunctionAlias
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionAlias.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionAlias::FunctionAlias()
    : Function( "Alias" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionAlias::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionAlias::NeedsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionAlias::CreateNode() const
{
    return FNEW( AliasNode );
}

//------------------------------------------------------------------------------
