// FunctionCompiler
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionCompiler::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    CompilerNode * compilerNode = nodeGraph.CreateCompilerNode(m_AliasForFunction);

    if ( !PopulateProperties( nodeGraph, funcStartIter, compilerNode ) )
    {
        return false;
    }

    if ( !compilerNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
