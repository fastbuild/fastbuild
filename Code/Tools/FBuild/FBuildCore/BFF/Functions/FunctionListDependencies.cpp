// FunctionListDependencies
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionListDependencies.h"
#include "Tools/FBuild/FBuildCore/Graph/ListDependenciesNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionListDependencies::FunctionListDependencies()
    : Function( "ListDependencies" )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
/*virtual*/ FunctionListDependencies::~FunctionListDependencies() = default;

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionListDependencies::AcceptsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionListDependencies::CreateNode() const
{
    return FNEW( ListDependenciesNode );
}

//------------------------------------------------------------------------------
