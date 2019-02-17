// FunctionUnity
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionUnity.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"

// Core
#include "Core/Strings/AStackString.h"
#include "Core/FileIO/PathUtils.h"

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
