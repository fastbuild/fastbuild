// FunctionDLL
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionDLL.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionDLL::FunctionDLL()
: FunctionExecutable()
{
    // override name set by FunctionExecutable base class
    m_Name =  "DLL";
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionDLL::CreateNode() const
{
    return FNEW( DLLNode );
}

//------------------------------------------------------------------------------
