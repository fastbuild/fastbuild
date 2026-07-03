// DTLTOGraphBuilder
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "DTLTOGraphBuilder.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/ReflectionInfo.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
DTLTOGraphBuilder::DTLTOGraphBuilder( NodeGraph & nodeGraph )
    : m_NodeGraph( nodeGraph )
{
}

// BuildGraph
//------------------------------------------------------------------------------
Node * DTLTOGraphBuilder::BuildGraph( const DTLTOData & data, const AString & aliasName )
{
    if ( data.m_Compiler.IsEmpty() )
    {
        FLOG_ERROR( "DTLTO: no compiler specified" );
        return nullptr;
    }
    if ( data.m_Jobs.IsEmpty() )
    {
        FLOG_ERROR( "DTLTO: no jobs to build" );
        return nullptr;
    }
    if ( m_NodeGraph.FindNode( aliasName ) )
    {
        FLOG_ERROR( "DTLTO: target '%s' already exists", aliasName.Get() );
        return nullptr;
    }

    // Build the job nodes
    StackArray<Node *> jobNodes;
    for ( const DTLTOData::Job & job : data.m_Jobs )
    {
        Node * execNode = CreateExecNodeForJob( data, job );
        if ( execNode == nullptr )
        {
            return nullptr; // CreateExecNodeForJob will have emitted an error
        }
        jobNodes.Append( execNode );
    }

    // group all jobs under a single root to build
    AliasNode * root = m_NodeGraph.CreateNode<AliasNode>( aliasName );

    const ReflectedProperty * targetsProp =
        root->GetReflectionInfoV()->GetReflectedProperty( AStackString( "Targets" ) );
    ASSERT( targetsProp );
    targetsProp->GetPtrToArray<Node *>( root )->Append( jobNodes );

    if ( !root->Initialize( m_NodeGraph, nullptr, Function::Find( AStackString( "Alias" ) ) ) )
    {
        return nullptr;
    }

    return root;
}

// CreateExecNodeForJob
//------------------------------------------------------------------------------
Node * DTLTOGraphBuilder::CreateExecNodeForJob( const DTLTOData & data, const DTLTOData::Job & job )
{
    // node name is the primary output file
    AStackString<512> nodeName;
    NodeGraph::CleanPath( job.m_Outputs[ 0 ], nodeName );
    // TODO: support multiple outputs? (DTLTO JSON can have multiple outputs)

    // TODO: could check outputs for uniqueness?

    AStackString<> arguments;
    BuildArgumentsString( data.m_CommonArgs, job.m_Args, arguments );

    ExecNode * execNode = m_NodeGraph.CreateNode<ExecNode>( nodeName );

    const ReflectionInfo * ri = execNode->GetReflectionInfoV();
    VERIFY( ri->SetProperty( execNode, "ExecExecutable", data.m_Compiler ) );
    VERIFY( ri->SetProperty( execNode, "ExecArguments", arguments ) );
    VERIFY( ri->SetProperty( execNode, "ExecInput", job.m_Inputs ) );
    VERIFY( ri->SetProperty( execNode, "ExecUseStdOutAsOutput", false ) );

    if ( !execNode->Initialize( m_NodeGraph, nullptr, Function::Find( AStackString( "Exec" ) ) ) )
    {
        return nullptr;
    }

    return execNode;
}

// BuildArgumentsString
//------------------------------------------------------------------------------
/*static*/ void DTLTOGraphBuilder::BuildArgumentsString( const Array<AString> & commonArgs,
                                                         const Array<AString> & jobArgs,
                                                         AString & outArguments )
{
    outArguments.Clear();
    for ( const AString & arg : commonArgs )
    {
        AppendQuotedArg( arg, outArguments );
        outArguments += ' ';
    }
    for ( const AString & arg : jobArgs )
    {
        AppendQuotedArg( arg, outArguments );
        outArguments += ' ';
    }
    if ( outArguments.IsEmpty() == false )
    {
        outArguments.SetLength( outArguments.GetLength() - 1 ); // remove trailing space
    }
}

// AppendQuotedArg
//------------------------------------------------------------------------------
/*static*/ void DTLTOGraphBuilder::AppendQuotedArg( const AString & arg, AString & out )
{
    const bool hasQuote = ( arg.Find( '"' ) != nullptr );
    const bool needsQuotes = arg.IsEmpty() ||
                             hasQuote ||
                             arg.Find( ' ' ) ||
                             arg.Find( '\t' );
    if ( !needsQuotes )
    {
        out += arg;
        return;
    }

    out += '"';
    if ( !hasQuote )
    {
        out += arg;
    }
    else
    {
        // escape embedded quotes
        for ( const char * pos = arg.Get(); pos != arg.GetEnd(); ++pos )
        {
            if ( *pos == '"' )
            {
                out += '\\';
            }
            out += *pos;
        }
    }
    out += '"';
}

//------------------------------------------------------------------------------
