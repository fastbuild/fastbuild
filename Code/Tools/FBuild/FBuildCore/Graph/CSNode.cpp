// CSNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CSNode.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( CSNode, Node, MetaName( "CompilerOutput" ) + MetaFile() )
    REFLECT(        m_Compiler,                     "Compiler",                     MetaFile() + MetaAllowNonFile() )
    REFLECT(        m_CompilerOptions,              "CompilerOptions",              MetaNone() )
    REFLECT(        m_CompilerOutput,               "CompilerOutput",               MetaFile() )
    REFLECT_ARRAY(  m_CompilerInputPath,            "CompilerInputPath",            MetaOptional() + MetaPath() )
    REFLECT(        m_CompilerInputPathRecurse,     "CompilerInputPathRecurse",     MetaOptional() )
    REFLECT_ARRAY(  m_CompilerInputPattern,         "CompilerInputPattern",         MetaOptional() )
    REFLECT_ARRAY(  m_CompilerInputExcludePath,     "CompilerInputExcludePath",     MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_CompilerInputExcludedFiles,   "CompilerInputExcludedFiles",   MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY(  m_CompilerInputExcludePattern,  "CompilerInputExcludePattern",  MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY(  m_CompilerInputFiles,           "CompilerInputFiles",           MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_CompilerReferences,           "CompilerReferences",           MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,      "PreBuildDependencies",         MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT(        m_NumCompilerInputFiles,        "NumCompilerInputFiles",        MetaHidden() )
    REFLECT(        m_NumCompilerReferences,        "NumCompilerReferences",        MetaHidden() )
REFLECT_END( CSNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
CSNode::CSNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
    , m_CompilerInputPathRecurse( true )
    , m_NumCompilerInputFiles( 0 )
    , m_NumCompilerReferences( 0 )
{
    m_CompilerInputPattern.EmplaceBack( "*.cs" );
    m_Type = CS_NODE;
    m_LastBuildTimeMs = 5000; // higher default than a file node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool CSNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .Compiler
    CompilerNode * compilerNode( nullptr );
    if ( !Function::GetCompilerNode( nodeGraph, iter, function, m_Compiler, compilerNode ) )
    {
        return false; // GetCompilerNode will have emitted an error
    }

    // Compiler must be C# compiler
    if ( compilerNode->GetCompilerFamily() != CompilerNode::CompilerFamily::CSHARP )
    {
        Error::Error_1504_CSAssemblyRequiresACSharpCompiler( iter, function );
        return false;
    }

    // .CompilerInputPath
    Dependencies compilerInputPath;
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_CompilerInputPath,
                                              m_CompilerInputExcludePath,
                                              m_CompilerInputExcludedFiles,
                                              m_CompilerInputExcludePattern,
                                              m_CompilerInputPathRecurse,
                                              false, // Don't include read-only status in hash
                                              &m_CompilerInputPattern,
                                              "CompilerInputPath",
                                              compilerInputPath ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }
    ASSERT( compilerInputPath.GetSize() == m_CompilerInputPath.GetSize() ); // No need to store since they should be the same

    // .CompilerInputFiles
    Dependencies compilerInputFiles;
    if ( !Function::GetFileNodes( nodeGraph, iter, function, m_CompilerInputFiles, "CompilerInputFiles", compilerInputFiles ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    m_NumCompilerInputFiles = (uint32_t)compilerInputFiles.GetSize();

    // .CompilerReferences
    Dependencies compilerReferences;
    if ( !Function::GetFileNodes( nodeGraph, iter, function, m_CompilerReferences, ".CompilerReferences", compilerReferences ) )
    {
        return false; // GetNodeList will have emitted an error
    }
    m_NumCompilerReferences = (uint32_t)compilerReferences.GetSize();

    // Store dependencies
    m_StaticDependencies.SetCapacity( 1 + m_CompilerInputPath.GetSize() + m_NumCompilerInputFiles + m_NumCompilerReferences );
    m_StaticDependencies.EmplaceBack( compilerNode );
    m_StaticDependencies.Append( compilerInputPath );
    m_StaticDependencies.Append( compilerInputFiles );
    m_StaticDependencies.Append( compilerReferences );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CSNode::~CSNode() = default;

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool CSNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool /*forceClean*/ )
{
    // clear dynamic deps from previous passes
    m_DynamicDependencies.Clear();

    // get the result of the directory lists and depend on those
    const size_t startIndex = 1; // Skip Compiler
    const size_t endIndex =  ( 1 + m_CompilerInputPath.GetSize() );
    for ( size_t i=startIndex; i<endIndex; ++i )
    {
        const Node * n = m_StaticDependencies[ i ].GetNode();

        ASSERT( n->GetType() == Node::DIRECTORY_LIST_NODE );

        // get the list of files
        const DirectoryListNode * dln = n->CastTo< DirectoryListNode >();
        const Array< FileIO::FileInfo > & files = dln->GetFiles();
        m_DynamicDependencies.SetCapacity( m_DynamicDependencies.GetSize() + files.GetSize() );
        for ( const FileIO::FileInfo & file : files )
        {
            // Create the file node (or find an existing one)
            Node * sn = nodeGraph.FindNode( file.m_Name );
            if ( sn == nullptr )
            {
                sn = nodeGraph.CreateFileNode( file.m_Name );
            }
            else if ( sn->IsAFile() == false )
            {
                FLOG_ERROR( "CSAssembly() .CompilerInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                return false;
            }

            m_DynamicDependencies.EmplaceBack( sn );
        }
        continue;
    }

    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CSNode::DoBuild( Job * job )
{
    // Format compiler args string
    Args fullArgs;
    if ( !BuildArgs( fullArgs ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
    }

    // use the exe launch dir as the working dir
    const char * workingDir = nullptr;

    const char * environment = FBuild::Get().GetEnvironmentString();

    EmitCompilationMessage( fullArgs );

    // spawn the process
    Process p( FBuild::Get().GetAbortBuildPointer() );
    if ( p.Spawn( GetCompiler()->GetExecutable().Get(), fullArgs.GetFinalArgs().Get(),
                  workingDir, environment ) == false )
    {
        if ( p.HasAborted() )
        {
            return NODE_RESULT_FAILED;
        }

        FLOG_ERROR( "Failed to spawn process to build '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // capture all of the stdout and stderr
    AString memOut;
    AString memErr;
    p.ReadAllData( memOut, memErr );

    // Get result
    const int result = p.WaitForExit();
    if ( p.HasAborted() )
    {
        return NODE_RESULT_FAILED;
    }

    const bool ok = ( result == 0 );

    // Show output if desired
    const bool showOutput = ( ok == false ) ||
                            FBuild::Get().GetOptions().m_ShowCommandOutput;
    if ( showOutput )
    {
        Node::DumpOutput( job, memOut );
        Node::DumpOutput( job, memErr );
    }

    if ( !ok )
    {
        FLOG_ERROR( "Failed to build Object. Error: %s Target: '%s'", ERROR_STR( result ), GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // record new file time
    RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// GetCompiler
//------------------------------------------------------------------------------
CompilerNode* CSNode::GetCompiler() const
{
    return m_StaticDependencies[0].GetNode()->CastTo< CompilerNode >();
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void CSNode::EmitCompilationMessage( const Args & fullArgs ) const
{
    // print basic or detailed output, depending on options
    // we combine everything into one string to ensure it is contiguous in
    // the output
    AStackString<> output;
    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        output += "C#: ";
        output += GetName();
        output += '\n';
    }
    if ( FBuild::Get().GetOptions().m_ShowCommandLines )
    {
        output += GetCompiler()->GetExecutable();
        output += ' ';
        output += fullArgs.GetRawArgs();
        output += '\n';
    }
    FLOG_OUTPUT( output );
}

// BuildArgs
//------------------------------------------------------------------------------
bool CSNode::BuildArgs( Args & fullArgs ) const
{
    // split into tokens
    Array< AString > tokens( 1024, true );
    m_CompilerOptions.Tokenize( tokens );

    AStackString<> quote( "\"" );

    const AString * const end = tokens.End();
    for ( const AString * it = tokens.Begin(); it!=end; ++it )
    {
        const AString & token = *it;
        if ( token.EndsWith( "%1" ) )
        {
            // handle /Option:%1 -> /Option:A /Option:B /Option:C
            AStackString<> pre;
            if ( token.GetLength() > 2 )
            {
                pre.Assign( token.Get(), token.GetEnd() - 2 );
            }

            // concatenate files, unquoted
            GetInputFiles( fullArgs, pre, AString::GetEmpty() );
        }
        else if ( token.EndsWith( "\"%1\"" ) )
        {
            // handle /Option:"%1" -> /Option:"A" /Option:"B" /Option:"C"
            AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote

            // concatenate files, quoted
            GetInputFiles( fullArgs, pre, quote );
        }
        else if ( token.EndsWith( "%2" ) )
        {
            // handle /Option:%2 -> /Option:A
            if ( token.GetLength() > 2 )
            {
                fullArgs += AStackString<>( token.Get(), token.GetEnd() - 2 );
            }
            fullArgs += m_Name;
        }
        else if ( token.EndsWith( "\"%2\"" ) )
        {
            // handle /Option:"%2" -> /Option:"A"
            AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote
            fullArgs += pre;
            fullArgs += m_Name;
            fullArgs += '"'; // post
        }
        else if ( token.EndsWith( "%3" ) )
        {
            // handle /Option:%3 -> /Option:A,B,C
            AStackString<> pre( token.Get(), token.GetEnd() - 2 );
            fullArgs += pre;

            // concatenate files, unquoted
            GetExtraRefs( fullArgs, AString::GetEmpty(), AString::GetEmpty() );
        }
        else if ( token.EndsWith( "\"%3\"" ) )
        {
            // handle /Option:"%3" -> /Option:"A","B","C"
            AStackString<> pre( token.Get(), token.GetEnd() - 4 );
            fullArgs += pre;

            // concatenate files, quoted
            GetExtraRefs( fullArgs, quote, quote );
        }
        else
        {
            fullArgs += token;
        }

        fullArgs.AddDelimiter();
    }

    // Handle all the special needs of args
    if ( fullArgs.Finalize( m_CompilerOptions, GetName(), ArgsResponseFileMode::IF_NEEDED ) == false )
    {
        return false; // Finalize will have emitted an error
    }

    return true;
}

// GetInputFiles
//------------------------------------------------------------------------------
void CSNode::GetInputFiles( Args & fullArgs, const AString & pre, const AString & post ) const
{
    bool first = true;

    // Add the explicitly listed files
    const size_t startIndex = ( 1 + m_CompilerInputPath.GetSize() ); // Skip compiler and input paths
    const size_t endIndex = ( startIndex + m_NumCompilerInputFiles );
    for ( size_t i=startIndex; i<endIndex; ++i )
    {
        if ( !first )
        {
            fullArgs.AddDelimiter();
        }
        fullArgs += pre;
        fullArgs += m_StaticDependencies[ i ].GetNode()->GetName();
        fullArgs += post;
        first = false;
    }

    // Add the files discovered from directory listings
    for ( const Dependency & dep : m_DynamicDependencies )
    {
        if ( !first )
        {
            fullArgs.AddDelimiter();
        }
        fullArgs += pre;
        fullArgs += dep.GetNode()->GetName();
        fullArgs += post;
        first = false;
    }
}

// GetExtraRefs
//------------------------------------------------------------------------------
void CSNode::GetExtraRefs( Args & fullArgs, const AString & pre, const AString & post ) const
{
    bool first = true;
    const size_t startIndex = ( 1 + m_CompilerInputPath.GetSize() + m_NumCompilerInputFiles ); // Skip compiler, input paths and files
    const size_t endIndex = ( startIndex + m_NumCompilerReferences );
    ASSERT( endIndex == m_StaticDependencies.GetSize() ); // References are last
    for ( size_t i=startIndex; i<endIndex; ++i )
    {
        if ( !first )
        {
            fullArgs += ',';
        }
        fullArgs += pre;
        fullArgs += m_StaticDependencies[ i ].GetNode()->GetName();
        fullArgs += post;
        first = false;
    }
}

//------------------------------------------------------------------------------
