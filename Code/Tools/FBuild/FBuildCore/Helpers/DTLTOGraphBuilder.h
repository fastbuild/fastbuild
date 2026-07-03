// DTLTOGraphBuilder - Synthesize a NodeGraph from parsed LLVM DTLTO data
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Helpers/DTLTOData.h"

#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Node;
class NodeGraph;

// DTLTOGraphBuilder
//------------------------------------------------------------------------------
class DTLTOGraphBuilder
{
public:
    explicit DTLTOGraphBuilder( NodeGraph & nodeGraph );

    DTLTOGraphBuilder( const DTLTOGraphBuilder & other ) = delete;
    DTLTOGraphBuilder & operator=( const DTLTOGraphBuilder & other ) = delete;

    // build an ExecNode-per-job graph, grouped under an Alias.
    Node * BuildGraph( const DTLTOData & data, const AString & aliasName );

private:
    Node * CreateExecNodeForJob( const DTLTOData & data, const DTLTOData::Job & job );

    static void BuildArgumentsString( const Array<AString> & commonArgs,
                                      const Array<AString> & jobArgs,
                                      AString & outArguments );
    static void AppendQuotedArg( const AString & arg, AString & out );

    NodeGraph & m_NodeGraph;
};

//------------------------------------------------------------------------------
