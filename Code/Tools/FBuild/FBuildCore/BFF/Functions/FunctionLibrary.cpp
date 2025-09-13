// FunctionLibrary
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionLibrary.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionLibrary::FunctionLibrary()
    : FunctionObjectList()
{
    m_Name = "Library";
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::NeedsHeader() const
{
    return false;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionLibrary::CreateNode() const
{
    return FNEW( LibraryNode );
}

//------------------------------------------------------------------------------
