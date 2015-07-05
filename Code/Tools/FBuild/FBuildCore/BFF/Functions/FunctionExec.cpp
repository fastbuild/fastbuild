// FunctionTest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionExec.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionExec::FunctionExec()
: Function( "Exec" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionExec::AcceptsHeader() const
{
	return true;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionExec::Commit( const BFFIterator & funcStartIter ) const
{
	// make sure all required variables are defined
	const BFFVariable * outputV;
	const BFFVariable * executableV;
	const BFFVariable * inputV;
	const BFFVariable * argsV;
	const BFFVariable * workingDirV;
	int32_t expectedReturnCode;
	if ( !GetString( funcStartIter, outputV,		".ExecOutput", true ) ||
		 !GetString( funcStartIter, executableV,	".ExecExecutable", true ) ||
		 !GetString( funcStartIter, inputV,			".ExecInput", true ) ||
		 !GetString( funcStartIter, argsV,			".ExecArguments" ) ||
		 !GetString( funcStartIter, workingDirV,	".ExecWorkingDir" ) ||
		 !GetInt( funcStartIter, expectedReturnCode, ".ExecReturnCode", 0, false ) )
	{
		return false;
	}

	// check for duplicates
	NodeGraph & ng = FBuild::Get().GetDependencyGraph();
	if ( ng.FindNode( outputV->GetString() ) != nullptr )
	{
		Error::Error_1100_AlreadyDefined( funcStartIter, this, outputV->GetString() );
		return false;
	}

	// Pre-build dependencies
	Dependencies preBuildDependencies;
	if ( !GetNodeList( funcStartIter, ".PreBuildDependencies", preBuildDependencies, false ) )
	{
		return false; // GetNodeList will have emitted an error
	}

	Dependencies additionalDependencies;
	if (!GetNodeList(funcStartIter, ".ExecAdditionalDependencies", additionalDependencies, false))
	{
		return false; // GetNodeList will have emitted an error
	}

	// get executable node
	Node * exeNode = ng.FindNode( executableV->GetString() );
	if ( exeNode == nullptr )
	{
		exeNode = ng.CreateFileNode( executableV->GetString() );
	}
	else if ( exeNode->IsAFile() == false )
	{
		Error::Error_1103_NotAFile( funcStartIter, this, "ExecExecutable", exeNode->GetName(), exeNode->GetType() );
		return false;
	}

	// source node
	Node * inputNode = ng.FindNode( inputV->GetString() );
	if ( inputNode == nullptr )
	{
		inputNode = ng.CreateFileNode( inputV->GetString() );
	}
	else if ( inputNode->IsAFile() == false )
	{
		Error::Error_1103_NotAFile( funcStartIter, this, "ExecInput", exeNode->GetName(), exeNode->GetType() );
		return false;
	}

	// optional args
	const AString & arguments(	argsV ?			argsV->GetString()		: AString::GetEmpty() );
	const AString & workingDir( workingDirV ?	workingDirV->GetString(): AString::GetEmpty() );

	// create the TestNode
	Node * outputNode = ng.CreateExecNode( outputV->GetString(), 
										   (FileNode *)inputNode,
										   (FileNode *)exeNode,
										   arguments,
										   workingDir, 
										   expectedReturnCode,
										   preBuildDependencies,
										   additionalDependencies);
	
	return ProcessAlias( funcStartIter, outputNode );
}

//------------------------------------------------------------------------------
