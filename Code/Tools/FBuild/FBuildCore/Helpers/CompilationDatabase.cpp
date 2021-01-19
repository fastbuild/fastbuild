// CompilationDatabase
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
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

// system
#include <string.h> // for memset

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilationDatabase::CompilationDatabase()
: m_Output( 4 * 1024 * 1024 )
{
    m_DirectoryEscaped = FBuild::Get().GetWorkingDir();
    JSONEscape( m_DirectoryEscaped );
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

    VisitNodes( nodeGraph, dependencies, visited );

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
void CompilationDatabase::VisitNodes( const NodeGraph & nodeGraph,
                                      const Dependencies & dependencies,
                                      Array< bool > & visited )
{
    for ( const Dependency & dep : dependencies )
    {
        const Node * node = dep.GetNode();

        // Skip already visited nodes
        const uint32_t nodeIndex = node->GetIndex();
        ASSERT( nodeIndex != INVALID_NODE_INDEX );
        if ( visited[ nodeIndex ] )
        {
            continue;
        }
        visited[ nodeIndex ] = true;

        VisitNodes( nodeGraph, node->GetPreBuildDependencies(), visited );
        VisitNodes( nodeGraph, node->GetStaticDependencies(), visited );
        VisitNodes( nodeGraph, node->GetDynamicDependencies(), visited );

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
                HandleObjectListNode( nodeGraph, node->CastTo< ObjectListNode >() );
                break;
            }
            case Node::LIBRARY_NODE:
            {
                HandleObjectListNode( nodeGraph, node->CastTo< LibraryNode >() );
                break;
            }
            default: break;
        }
    }
}

// HandleObjectListNode
//------------------------------------------------------------------------------
void CompilationDatabase::HandleObjectListNode( const NodeGraph & nodeGraph, ObjectListNode * node )
{
    ObjectListContext ctx;
    ctx.m_DB = this;
    ctx.m_ObjectListNode = node;

    const AString & compilerName = node->GetCompiler();
    const Node * compilerNode = nodeGraph.FindNode( compilerName );
    const bool isMSVC = compilerNode &&
                        ( compilerNode->GetType() == Node::COMPILER_NODE ) &&
                        ( compilerNode->CastTo< CompilerNode >()->GetCompilerFamily() == CompilerNode::MSVC );

    ctx.m_CompilerEscaped = compilerName;
    JSONEscape( ctx.m_CompilerEscaped );

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
        Unquote( argument );
        JSONEscape( argument );
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
    JSONEscape( inputFileEscaped );

    AStackString<> outputFileEscaped;
    ctx->m_ObjectListNode->GetObjectFileName( inputFile, baseDir, outputFileEscaped );
    JSONEscape( outputFileEscaped );

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
            arg.Append( argument.Get(), (size_t)( found - argument.Get() ) );
            arg.Append( inputFileEscaped );
            arg.Append( found + 2, (size_t)( argument.GetEnd() - ( found + 2 ) ) );
            m_Output += ", \"";
            m_Output += arg;
            m_Output += "\"";
            continue;
        }

        found = argument.Find( "%2" );
        if ( found )
        {
            AStackString<> arg;
            arg.Append( argument.Get(), (size_t)( found - argument.Get() ) );
            arg.Append( outputFileEscaped );
            arg.Append( found + 2, (size_t)( argument.GetEnd() - ( found + 2 ) ) );
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


// JSONEscape
//------------------------------------------------------------------------------
/*static*/ void CompilationDatabase::JSONEscape( AString & string )
{
    // Build result in a temporary buffer
    AStackString< 8192 > temp;

    const char * end = string.GetEnd();
    for ( const char * pos = string.Get(); pos != end; ++pos )
    {
        const char c = *pos;

        // congrol character?
        if ( c <= 0x1F )
        {
            // escape with backslash if possible
            if ( c == '\b' ) { temp += "\\b"; continue; }
            if ( c == '\t' ) { temp += "\\t"; continue; }
            if ( c == '\n' ) { temp += "\\n"; continue; }
            if ( c == '\f' ) { temp += "\\f"; continue; }
            if ( c == '\r' ) { temp += "\\r"; continue; }

            // escape with codepoint
            temp.AppendFormat( "\\u%04X", c );
            continue;
        }
        else if ( c == '\"' )
        {
            // escape quotes
            temp += "\\\"";
            continue;
        }
        else if ( c == '\\' )
        {
            // escape backslashes
            temp += "\\\\";
            continue;
        }

        // char does not need escpaing
        temp += c;
    }

    // store final result
    string = temp;
}

// Unquote
//------------------------------------------------------------------------------
/*static*/ void CompilationDatabase::Unquote( AString & string )
{
    const char * src = string.Get();
    const char * end = string.GetEnd();
    char * dst = string.Get();

    char quoteChar = 0;
    for ( ; src < end; ++src )
    {
        const char c = *src;
        if ( ( c == '"' ) || ( c == '\'' ) )
        {
            if ( quoteChar == 0 )
            {
                // opening quote, ignore it
                quoteChar = c;
                continue;
            }
            else if ( quoteChar == c )
            {
                // closing quote, ignore it
                quoteChar = 0;
                continue;
            }
            else
            {
                // quote of the 'other' type - consider as part of token
            }
        }
        *dst++ = c;
    }

    string.SetLength( (uint32_t)( dst - string.Get() ) );
}


//------------------------------------------------------------------------------
