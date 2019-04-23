// CompilationDatabase
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CompilationDatabase.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"

// Core
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilationDatabase::CompilationDatabase()
: m_Output( 4 * 1024 * 1024 )
{
    m_DirectoryEscaped = FBuild::Get().GetWorkingDir();
    m_DirectoryEscaped.JSONEscape();
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilationDatabase::~CompilationDatabase() = default;

// Generate
//------------------------------------------------------------------------------
const AString & CompilationDatabase::Generate( const NodeGraph & nodeGraph, Dependencies & dependencies )
{
    m_Output += "[\n";

    const size_t numNodes = nodeGraph.GetNodeCount();
    Array< bool > visited( numNodes, false );
    visited.SetSize( numNodes );
    memset( visited.Begin(), 0, numNodes );

    VisitNodes( dependencies, visited );

    // Remove last comma
    for ( uint32_t i = m_Output.GetLength() - 1; i != 0; --i )
    {
        const char c = m_Output[ i ];
        if ( c == ',' )
        {
            m_Output[ i ] = '\n';
            m_Output.SetLength( i + 1 );
            break;
        }
        if ( ( c != '\n' ) && ( c != '\r' ) && ( c != ' ' ) && ( c != '\t' ) )
        {
            break;
        }
    }

    m_Output += "]\n";

    return m_Output;
}

// VisitNodes
//------------------------------------------------------------------------------
void CompilationDatabase::VisitNodes( const Dependencies & dependencies, Array< bool > & visited )
{
    for ( const Dependency & dep : dependencies )
    {
        Node * node = dep.GetNode();

        // Skip already visited nodes
        uint32_t nodeIndex = node->GetIndex();
        ASSERT( nodeIndex != INVALID_NODE_INDEX );
        if ( visited[ nodeIndex ] )
        {
            continue;
        }
        visited[ nodeIndex ] = true;

        VisitNodes( node->GetPreBuildDependencies(), visited );
        VisitNodes( node->GetStaticDependencies(), visited );
        VisitNodes( node->GetDynamicDependencies(), visited );

        switch ( node->GetType() )
        {
            case Node::DIRECTORY_LIST_NODE:
            {
                // Build directory list node to populate its file list
                node->CastTo< DirectoryListNode >()->DoBuild( nullptr );
                break;
            }
            case Node::OBJECT_LIST_NODE:
            {
                HandleObjectListNode( node->CastTo< ObjectListNode >() );
                break;
            }
            case Node::LIBRARY_NODE:
            {
                HandleObjectListNode( node->CastTo< LibraryNode >() );
                break;
            }
            default: break;
        }
    }
}

// HandleObjectListNode
//------------------------------------------------------------------------------
void CompilationDatabase::HandleObjectListNode( ObjectListNode * node )
{
    ObjectListContext ctx;
    ctx.m_DB = this;
    ctx.m_ObjectListNode = node;

    const CompilerNode * compiler = node->GetCompiler();
    const bool isMSVC = ( compiler->GetCompilerFamily() == CompilerNode::MSVC );

    ctx.m_CompilerEscaped = compiler->GetExecutable();
    ctx.m_CompilerEscaped.JSONEscape();

    // Prepare arguments: tokenize, remove problematic arguments, remove extra quoting and escape.
    node->GetCompilerOptions().Tokenize( ctx.m_ArgumentsEscaped );
    for ( size_t i = 0; i < ctx.m_ArgumentsEscaped.GetSize(); ++i )
    {
        AString & argument = ctx.m_ArgumentsEscaped[ i ];
        if ( isMSVC && ObjectNode::IsStartOfCompilerArg_MSVC( argument, "analyze" ) )
        {
            // Clang doesn't recognize /analyze as a compiler option, treats it an input file and
            // later complains that /Fo option can't be used with multiple input files.
            // To avoid these problems we strip this option.
            ctx.m_ArgumentsEscaped.EraseIndex( i-- );
            continue;
        }
        argument.Unquote();
        argument.JSONEscape();
    }

    node->EnumerateInputFiles( &CompilationDatabase::HandleInputFile, &ctx );
}

// HandleInputFile
//------------------------------------------------------------------------------
/*static*/ void CompilationDatabase::HandleInputFile( const AString & inputFile, const AString & baseDir, void * userData )
{
    ObjectListContext * ctx = static_cast< ObjectListContext * >( userData );
    ctx->m_DB->HandleInputFile( inputFile, baseDir, ctx );
}

// HandleInputFile
//------------------------------------------------------------------------------
void CompilationDatabase::HandleInputFile( const AString & inputFile, const AString & baseDir, ObjectListContext * ctx )
{
    AStackString<> inputFileEscaped;
    inputFileEscaped = inputFile;
    inputFileEscaped.JSONEscape();

    AStackString<> outputFileEscaped;
    ctx->m_ObjectListNode->GetObjectFileName( inputFile, baseDir, outputFileEscaped );
    outputFileEscaped.JSONEscape();

    m_Output += "  {\n    \"directory\": \"";
    m_Output += m_DirectoryEscaped;
    m_Output += "\",\n    \"file\": \"";
    m_Output += inputFileEscaped;
    m_Output += "\",\n    \"output\": \"";
    m_Output += outputFileEscaped;
    m_Output += "\",\n    \"arguments\": [\"";
    m_Output += ctx->m_CompilerEscaped;
    m_Output += "\"";
    for ( const AString & argument : ctx->m_ArgumentsEscaped )
    {
        const char * found = argument.Find( "%1" );
        if ( found )
        {
            AStackString<> arg;
            arg.Append( argument.Get(), found - argument.Get() );
            arg.Append( inputFileEscaped );
            arg.Append( found + 2, argument.GetEnd() - ( found + 2 ) );
            m_Output += ", \"";
            m_Output += arg;
            m_Output += "\"";
            continue;
        }

        found = argument.Find( "%2" );
        if ( found )
        {
            AStackString<> arg;
            arg.Append( argument.Get(), found - argument.Get() );
            arg.Append( outputFileEscaped );
            arg.Append( found + 2, argument.GetEnd() - ( found + 2 ) );
            m_Output += ", \"";
            m_Output += arg;
            m_Output += "\"";
            continue;
        }

        // TODO: Replace %3 and %4 with proper values.
        //       This is low priority as there are currently no tools that want these values.

        // Regular argument
        m_Output += ", \"";
        m_Output += argument;
        m_Output += "\"";
    }
    m_Output += "]\n  },\n";
}

//------------------------------------------------------------------------------
