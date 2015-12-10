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
/*virtual*/ bool FunctionCompiler::Commit( const BFFIterator & funcStartIter ) const
{
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	AStackString<> name;
	if ( GetNameForNode( funcStartIter, CompilerNode::GetReflectionInfoS(), name ) == false )
	{
		return false;
	}

	if ( ng.FindNode( name ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
		return false;
	}

	CompilerNode * compilerNode = ng.CreateCompilerNode( name );

	if ( !PopulateProperties( funcStartIter, compilerNode ) )
	{
		return false;
	}

	if ( !compilerNode->Initialize( funcStartIter, this ) )
    {
        return false;
    }

	// handle alias creation
	return ProcessAlias( funcStartIter, compilerNode );
}

//------------------------------------------------------------------------------
