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
#ifndef USE_NODE_REFLECTION
/*virtual*/ bool FunctionCompiler::Commit( const BFFIterator & funcStartIter ) const
{
	const BFFVariable * executableV;
	if ( !GetString( funcStartIter, executableV, ".Executable", true ) )
	{
		return false; // GetString will have emitted error
	}

	// path cleanup
	AStackString<> exe;
	NodeGraph::CleanPath( executableV->GetString(), exe );

	// exe must be a file, not a path
	if ( PathUtils::IsFolderPath( exe ) )
	{
		Error::Error_1105_PathNotAllowed( funcStartIter, this, ".Executable", exe );
		return false;
	}

	//
	Dependencies extraFiles( 32, true );
	if ( !GetNodeList( funcStartIter, ".ExtraFiles", extraFiles, false ) ) // optional
	{
		return false; // GetNodeList will have emitted an error
	}

	// get executable node
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	if ( ng.FindNode( exe ) )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, exe );
		return false;
	}

	// create our node
	const bool allowDistribution = true;
	Node * compilerNode = ng.CreateCompilerNode( exe, extraFiles, allowDistribution );

	// handle alias creation
	return ProcessAlias( funcStartIter, compilerNode );
}
#endif

// Commit
//------------------------------------------------------------------------------
#ifdef USE_NODE_REFLECTION
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
#endif

//------------------------------------------------------------------------------
