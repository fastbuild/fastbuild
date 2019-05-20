// FunctionExecutable
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionExecutable.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionExecutable::FunctionExecutable()
: Function( "Executable" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionExecutable::AcceptsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionExecutable::CreateNode() const
{
    return FNEW( ExeNode );
}

//------------------------------------------------------------------------------
