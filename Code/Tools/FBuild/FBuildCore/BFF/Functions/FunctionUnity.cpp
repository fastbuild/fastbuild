// FunctionUnity
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionUnity.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"

// UnityNode
//------------------------------------------------------------------------------
class UnityNode;

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionUnity::FunctionUnity()
: Function( "Unity" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUnity::NeedsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionUnity::CreateNode() const
{
    return FNEW( UnityNode );
}

//------------------------------------------------------------------------------
