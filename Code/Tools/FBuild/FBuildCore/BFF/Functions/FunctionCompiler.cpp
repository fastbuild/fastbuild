// FunctionCompiler
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionCompiler.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionCompiler::FunctionCompiler()
: Function( "Compiler" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCompiler::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCompiler::NeedsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionCompiler::CreateNode() const
{
    return FNEW( CompilerNode );
}

//------------------------------------------------------------------------------
