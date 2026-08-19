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
class CompilerNode;
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

    Node * BuildGraph( const DTLTOData & data, const AString & aliasName );

private:
    CompilerNode * CreateCompilerNode( const AString & compilerExe );
    Node * CreateObjectListForJob( const DTLTOData & data,
                                   const DTLTOData::Job & job,
                                   CompilerNode * compiler );

    static void BuildCompilerOptions( const Array<AString> & commonArgs,
                                      const Array<AString> & jobArgs,
                                      AString & outOptions,
                                      AString & outOptionsForCacheKey );

    NodeGraph & m_NodeGraph;
};

//------------------------------------------------------------------------------
