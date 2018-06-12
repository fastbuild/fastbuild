// FunctionExecutable
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionExecutable.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
//#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"

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
