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
/*virtual*/ bool FunctionExec::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    // make sure all required variables are defined
    const BFFVariable * outputV;
    const BFFVariable * executableV;
    const BFFVariable * argsV;
    const BFFVariable * workingDirV;
    int32_t expectedReturnCode;
    bool useStdOutAsOutput;
    if ( !GetString( funcStartIter, outputV,        ".ExecOutput", true ) ||
         !GetString( funcStartIter, executableV,    ".ExecExecutable", true ) ||
         !GetString( funcStartIter, argsV,          ".ExecArguments" ) ||
         !GetString( funcStartIter, workingDirV,    ".ExecWorkingDir" ) ||
         !GetInt( funcStartIter, expectedReturnCode, ".ExecReturnCode", 0, false ) ||
         !GetBool( funcStartIter, useStdOutAsOutput, ".ExecUseStdOutAsOutput", false, false))
    {
        return false;
    }

    // check for duplicates
    if ( nodeGraph.FindNode( outputV->GetString() ) != nullptr )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, outputV->GetString() );
        return false;
    }

    // Pre-build dependencies
    Dependencies preBuildDependencies;
    const bool allowCopyDirNodes = true;
    if ( !GetNodeList( nodeGraph, funcStartIter, ".PreBuildDependencies", preBuildDependencies, false, allowCopyDirNodes ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    // get executable node
    Node * exeNode = nodeGraph.FindNode( executableV->GetString() );
    if ( exeNode == nullptr )
    {
        exeNode = nodeGraph.CreateFileNode( executableV->GetString() );
    }
    else if ( exeNode->IsAFile() == false )
    {
        Error::Error_1103_NotAFile( funcStartIter, this, "ExecExecutable", exeNode->GetName(), exeNode->GetType() );
        return false;
    }

    // source node
    Dependencies inputNodes;
    if ( !GetNodeList( nodeGraph, funcStartIter, ".ExecInput", inputNodes, false ) )
    {
        return false; // GetNodeList will have emitted an error
    }
    else
    {
        // Make sure all nodes are files
        const Dependency * const end = inputNodes.End();
        for (const Dependency * it = inputNodes.Begin();
            it != end;
            ++it)
        {
            Node * node = it->GetNode();
            if (node->IsAFile() == false)
            {
                Error::Error_1103_NotAFile(funcStartIter, this, "ExecInput", node->GetName(), node->GetType());
                return false;
            }
        }
    }

    // optional args
    const AString & arguments(  argsV ?         argsV->GetString()      : AString::GetEmpty() );
    const AString & workingDir( workingDirV ?   workingDirV->GetString(): AString::GetEmpty() );

    // create the TestNode
    Node * outputNode = nodeGraph.CreateExecNode( outputV->GetString(),
                                           inputNodes,
                                           (FileNode *)exeNode,
                                           arguments,
                                           workingDir,
                                           expectedReturnCode,
                                           preBuildDependencies,
                                           useStdOutAsOutput);

    return ProcessAlias( nodeGraph, funcStartIter, outputNode );
}

//------------------------------------------------------------------------------
