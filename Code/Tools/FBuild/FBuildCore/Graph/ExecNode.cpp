// ExecNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "ExecNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Process.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
ExecNode::ExecNode( const AString & dstFileName,
                        const Dependencies & inputFiles,
                        FileNode * executable,
                        const AString & arguments,
                        const AString & workingDir,
                        int32_t expectedReturnCode,
                        const Dependencies & preBuildDependencies,
                        bool useStdOutAsOutput )
: FileNode( dstFileName, Node::FLAG_NONE )
, m_InputFiles( inputFiles )
, m_Executable( executable )
, m_Arguments( arguments )
, m_WorkingDir( workingDir )
, m_ExpectedReturnCode( expectedReturnCode )
, m_UseStdOutAsOutput( useStdOutAsOutput )
{
    ASSERT( executable );
    m_StaticDependencies.SetCapacity( m_InputFiles.GetSize() + 1 );
    m_StaticDependencies.Append(m_InputFiles);
    m_StaticDependencies.Append( Dependency( executable ) );
    m_Type = EXEC_NODE;

    m_PreBuildDependencies = preBuildDependencies;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ExecNode::~ExecNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult ExecNode::DoBuild( Job * job )
{
    // If the workingDir is empty, use the current dir for the process
    const char * workingDir = m_WorkingDir.IsEmpty() ? nullptr : m_WorkingDir.Get();

    // Format compiler args string
    AStackString< 4 * KILOBYTE > fullArgs;
    GetFullArgs(fullArgs);

    EmitCompilationMessage( fullArgs );

    // spawn the process
    Process p;
    bool spawnOK = p.Spawn( m_Executable->GetName().Get(),
                            fullArgs.Get(),
                            workingDir,
                            FBuild::Get().GetEnvironmentString() );

    if ( !spawnOK )
    {
        FLOG_ERROR( "Failed to spawn process for '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // capture all of the stdout and stderr
    AutoPtr< char > memOut;
    AutoPtr< char > memErr;
    uint32_t memOutSize = 0;
    uint32_t memErrSize = 0;
    p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

    ASSERT( !p.IsRunning() );
    // Get result
    int result = p.WaitForExit();

    // did the executable fail?
    if ( result != m_ExpectedReturnCode )
    {
        // something went wrong, print details
        Node::DumpOutput( job, memOut.Get(), memOutSize );
        Node::DumpOutput( job, memErr.Get(), memErrSize );

        FLOG_ERROR( "Execution failed (error %i) '%s'", result, GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    if ( m_UseStdOutAsOutput == true )
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

// Load
//------------------------------------------------------------------------------
/*static*/ Node * ExecNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>,  fileName );
    NODE_LOAD_DEPS( 0,          inputFiles );
    NODE_LOAD( AStackString<>,  executable );
    NODE_LOAD( AStackString<>,  arguments );
    NODE_LOAD( AStackString<>,  workingDir );
    NODE_LOAD( int32_t,         expectedReturnCode );
    NODE_LOAD_DEPS( 0,          preBuildDependencies );
    NODE_LOAD( bool,            useStdOutAsOutput);

    Node * execNode = nodeGraph.FindNode( executable );
    ASSERT( execNode ); // load/save logic should ensure the src was saved first
    ASSERT( execNode->IsAFile() );
    ExecNode * n = nodeGraph.CreateExecNode( fileName,
                                  inputFiles,
                                  (FileNode *)execNode,
                                  arguments,
                                  workingDir,
                                  expectedReturnCode,
                                  preBuildDependencies,
                                  useStdOutAsOutput );
    ASSERT( n );

    return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void ExecNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    NODE_SAVE_DEPS( m_InputFiles );
    NODE_SAVE( m_Executable->GetName() );
    NODE_SAVE( m_Arguments );
    NODE_SAVE( m_WorkingDir );
    NODE_SAVE( m_ExpectedReturnCode );
    NODE_SAVE_DEPS( m_PreBuildDependencies );
    NODE_SAVE( m_UseStdOutAsOutput );
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
                              m_Executable->GetName().Get(),
                              args.Get(),
                              m_WorkingDir.Get(),
                              m_ExpectedReturnCode );
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
    m_Arguments.Tokenize(tokens);

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
    bool first = true;
    const Dependency * const end = m_InputFiles.End();
    for (const Dependency * it = m_InputFiles.Begin();
        it != end;
        ++it)
    {
        if (!first)
        {
            fullArgs += ' ';
        }
        fullArgs += pre;
        fullArgs += it->GetNode()->GetName();
        fullArgs += post;
        first = false;
    }
}

//------------------------------------------------------------------------------
