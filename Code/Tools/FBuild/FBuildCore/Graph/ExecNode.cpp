// ExecNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ExecNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Process.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( ExecNode, Node, MetaName( "ExecOutput" ) + MetaFile() )
    REFLECT(        m_ExecExecutable,           "ExecExecutable",           MetaFile() )
    REFLECT_ARRAY(  m_ExecInput,                "ExecInput",                MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_ExecInputPath,            "ExecInputPath",            MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_ExecInputPattern,         "ExecInputPattern",         MetaOptional() )
    REFLECT(        m_ExecInputPathRecurse,     "ExecInputPathRecurse",     MetaOptional() )
    REFLECT_ARRAY(  m_ExecInputExcludePath,     "ExecInputExcludePath",     MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_ExecInputExcludedFiles,   "ExecInputExcludedFiles",   MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY(  m_ExecInputExcludePattern,  "ExecInputExcludePattern",  MetaOptional() )
    REFLECT(        m_ExecArguments,            "ExecArguments",            MetaOptional() )
    REFLECT(        m_ExecWorkingDir,           "ExecWorkingDir",           MetaOptional() + MetaPath() )
    REFLECT(        m_ExecReturnCode,           "ExecReturnCode",           MetaOptional() )
    REFLECT(        m_ExecUseStdOutAsOutput,    "ExecUseStdOutAsOutput",    MetaOptional() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,  "PreBuildDependencies",     MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT(        m_NumExecInputFiles,        "NumExecInputFiles",        MetaHidden() )
REFLECT_END( ExecNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
ExecNode::ExecNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
    , m_ExecReturnCode( 0 )
    , m_ExecUseStdOutAsOutput( false )
    , m_ExecInputPathRecurse( true )
{
    m_Type = EXEC_NODE;

    m_ExecInputPattern.Append( AStackString<>( "*.*" ) );
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool ExecNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .ExecExecutable
    Dependencies executable;
    if ( !Function::GetFileNode( nodeGraph, iter, function, m_ExecExecutable, "ExecExecutable", executable ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    ASSERT( executable.GetSize() == 1 ); // Should only be possible to be one

    // .ExecInput
    Dependencies execInputFiles;
    if ( !Function::GetFileNodes( nodeGraph, iter, function, m_ExecInput, "ExecInput", execInputFiles ) )
    {
        return false; // GetFileNodes will have emitted an error
    }
    m_NumExecInputFiles = (uint32_t)execInputFiles.GetSize();

    // .ExecInputPath
    Dependencies execInputPaths;
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_ExecInputPath,
                                              m_ExecInputExcludePath,
                                              m_ExecInputExcludedFiles,
                                              m_ExecInputExcludePattern,
                                              m_ExecInputPathRecurse,
                                              &m_ExecInputPattern,
                                              "ExecInputPath",
                                              execInputPaths ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }
    ASSERT( execInputPaths.GetSize() == m_ExecInputPath.GetSize() ); // No need to store count since they should be the same

    // Store Static Dependencies
    m_StaticDependencies.SetCapacity( 1 + m_NumExecInputFiles + execInputPaths.GetSize() );
    m_StaticDependencies.Append( executable );
    m_StaticDependencies.Append( execInputFiles );
    m_StaticDependencies.Append( execInputPaths );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ExecNode::~ExecNode() = default;

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool ExecNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool UNUSED( forceClean ) )
{
    // clear dynamic deps from previous passes
    m_DynamicDependencies.Clear();

    // get the result of the directory lists and depend on those
    const size_t startIndex = 1 + m_NumExecInputFiles; // Skip Compiler + ExecInputFiles
    const size_t endIndex =  ( 1 + m_NumExecInputFiles + m_ExecInputPath.GetSize() );
    for ( size_t i = startIndex; i < endIndex; ++i )
    {
        Node * n = m_StaticDependencies[ i ].GetNode();

        ASSERT( n->GetType() == Node::DIRECTORY_LIST_NODE );

        // get the list of files
        DirectoryListNode * dln = n->CastTo< DirectoryListNode >();
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
                FLOG_ERROR( "Exec() .ExecInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                return false;
            }

            m_DynamicDependencies.Append( Dependency( sn ) );
        }
    }

    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ExecNode::DoBuild( Job * job )
{
    // If the workingDir is empty, use the current dir for the process
    const char * workingDir = m_ExecWorkingDir.IsEmpty() ? nullptr : m_ExecWorkingDir.Get();

    // Format compiler args string
    AStackString< 4 * KILOBYTE > fullArgs;
    GetFullArgs(fullArgs);

    EmitCompilationMessage( fullArgs );

    // spawn the process
    Process p( FBuild::Get().GetAbortBuildPointer() );
    bool spawnOK = p.Spawn( GetExecutable()->GetName().Get(),
                            fullArgs.Get(),
                            workingDir,
                            FBuild::Get().GetEnvironmentString() );

    if ( !spawnOK )
    {
        if ( p.HasAborted() )
        {
            return NODE_RESULT_FAILED;
        }

        FLOG_ERROR( "Failed to spawn process for '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // capture all of the stdout and stderr
    AutoPtr< char > memOut;
    AutoPtr< char > memErr;
    uint32_t memOutSize = 0;
    uint32_t memErrSize = 0;
    p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

    // Get result
    int result = p.WaitForExit();
    if ( p.HasAborted() )
    {
        return NODE_RESULT_FAILED;
    }

    // did the executable fail?
    if ( result != m_ExecReturnCode )
    {
        // something went wrong, print details
        Node::DumpOutput( job, memOut.Get(), memOutSize );
        Node::DumpOutput( job, memErr.Get(), memErrSize );

        FLOG_ERROR( "Execution failed (error %i) '%s'", result, GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    if ( m_ExecUseStdOutAsOutput == true )
    {
        FileStream f;
        f.Open( m_Name.Get(), FileStream::WRITE_ONLY );
        if (memOut.Get())
        {
            f.WriteBuffer(memOut.Get(), memOutSize);
        }

        f.Close();
    }

    // update the file's "last modified" time
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    return NODE_RESULT_OK;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void ExecNode::EmitCompilationMessage( const AString & args ) const
{
    // basic info
    AStackString< 2048 > output;
    output += "Run: ";
    output += GetName();
    output += '\n';

    // verbose mode
    if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
    {
        AStackString< 1024 > verboseOutput;
        verboseOutput.Format( "%s %s\nWorkingDir: %s\nExpectedReturnCode: %i\n",
                              GetExecutable()->GetName().Get(),
                              args.Get(),
                              m_ExecWorkingDir.Get(),
                              m_ExecReturnCode );
        output += verboseOutput;
    }

    // output all at once for contiguousness
    FLOG_BUILD_DIRECT( output.Get() );
}

// GetFullArgs
//------------------------------------------------------------------------------
void ExecNode::GetFullArgs(AString & fullArgs) const
{
    // split into tokens
    Array< AString > tokens(1024, true);
    m_ExecArguments.Tokenize(tokens);

    AStackString<> quote("\"");

    const AString * const end = tokens.End();
    for (const AString * it = tokens.Begin(); it != end; ++it)
    {
        const AString & token = *it;
        if (token.EndsWith("%1"))
        {
            // handle /Option:%1 -> /Option:A /Option:B /Option:C
            AStackString<> pre;
            if (token.GetLength() > 2)
            {
                pre.Assign(token.Get(), token.GetEnd() - 2);
            }

            // concatenate files, unquoted
            GetInputFiles(fullArgs, pre, AString::GetEmpty());
        }
        else if (token.EndsWith("\"%1\""))
        {
            // handle /Option:"%1" -> /Option:"A" /Option:"B" /Option:"C"
            AStackString<> pre(token.Get(), token.GetEnd() - 3); // 3 instead of 4 to include quote

            // concatenate files, quoted
            GetInputFiles(fullArgs, pre, quote);
        }
        else if (token.EndsWith("%2"))
        {
            // handle /Option:%2 -> /Option:A
            if (token.GetLength() > 2)
            {
                fullArgs += AStackString<>(token.Get(), token.GetEnd() - 2);
            }
            fullArgs += GetName().Get();
        }
        else if (token.EndsWith("\"%2\""))
        {
            // handle /Option:"%2" -> /Option:"A"
            AStackString<> pre(token.Get(), token.GetEnd() - 3); // 3 instead of 4 to include quote
            fullArgs += pre;
            fullArgs += GetName().Get();
            fullArgs += '"'; // post
        }
        else
        {
            fullArgs += token;
        }

        fullArgs += ' ';
    }
}

// GetInputFiles
//------------------------------------------------------------------------------
void ExecNode::GetInputFiles(AString & fullArgs, const AString & pre, const AString & post) const
{
    bool first = true; // Handle comma separation
    for ( size_t i=1; i < m_StaticDependencies.GetSize(); ++i ) // Note: Skip first dep (exectuable)
    {
        const Dependency & dep = m_StaticDependencies[ i ];
        const Node * n = dep.GetNode();

        // Handle directory lists
        if ( n->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            DirectoryListNode * dln = n->CastTo< DirectoryListNode >();
            const Array< FileIO::FileInfo > & files = dln->GetFiles();
            for ( const FileIO::FileInfo & file : files )
            {
                if ( !first )
                {
                    fullArgs += ' ';
                }
                fullArgs += pre;
                fullArgs += file.m_Name;
                fullArgs += post;
                first = false;
            }
            continue;
        }

        if ( !first )
        {
            fullArgs += ' ';
        }
        fullArgs += pre;
        fullArgs += dep.GetNode()->GetName();
        fullArgs += post;
        first = false;
    }
}

//------------------------------------------------------------------------------
