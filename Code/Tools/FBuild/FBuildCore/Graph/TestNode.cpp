// TestNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Process.h"
#include "Core/Env/Env.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( TestNode, Node, MetaName( "TestOutput" ) + MetaFile() )
    REFLECT(       m_TestExecutable,   "TestExecutable",      MetaFile() )
    REFLECT_ARRAY( m_TestInput,        "TestInput",           MetaOptional() + MetaFile() )
    REFLECT_ARRAY( m_TestInputPath,    "TestInputPath",       MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_TestInputPattern, "TestInputPattern",    MetaOptional() )
    REFLECT(       m_TestInputPathRecurse,    "TestInputPathRecurse",  MetaOptional() )
    REFLECT(        m_TestInputPathRecurse,     "TestInputPathRecurse",     MetaOptional() )
    REFLECT_ARRAY( m_TestInputExcludePath,    "TestInputExcludePath",     MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_TestInputExcludedFiles,  "TestInputExcludedFiles",   MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY( m_TestInputExcludePattern, "TestInputExcludePattern",  MetaOptional() )
    REFLECT(       m_TestArguments,           "TestArguments",        MetaOptional() )
    REFLECT(       m_TestWorkingDir,          "TestWorkingDir",       MetaOptional() + MetaPath() )
    REFLECT(       m_TestTimeOut,             "TestTimeOut",          MetaOptional() + MetaRange( 0, 4 * 60 * 60 ) ) // 4hrs
    REFLECT(       m_TestAlwaysShowOutput,    "TestAlwaysShowOutput", MetaOptional() )
    REFLECT_ARRAY( m_PreBuildDependencyNames, "PreBuildDependencies", MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT(       m_NumTestInputFiles,          "NumTestInputFiles",          MetaHidden() )
REFLECT_END( TestNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
TestNode::TestNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NO_DELETE_ON_FAIL ) // keep output on test fail
    , m_TestExecutable()
    , m_TestArguments()
    , m_TestWorkingDir()
    , m_TestTimeOut( 0 )
    , m_TestAlwaysShowOutput( false )
    , m_TestInputPathRecurse( true )
{
    m_Type = Node::TEST_NODE;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool TestNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .TestExecutable
    Dependencies executable;
    if ( !Function::GetFileNode( nodeGraph, iter, function, m_TestExecutable, "TestExecutable", executable ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    ASSERT( executable.GetSize() == 1 ); // Should only be possible to be one

    // .TestInput
    Dependencies testInputFiles;
    if ( !Function::GetFileNodes( nodeGraph, iter, function, m_TestInput, "TestInput", testInputFiles ) )
    {
        return false; // GetFileNodes will have emitted an error
    }
    m_NumTestInputFiles = (uint32_t)testInputFiles.GetSize();

    // .TestInputPath
    Dependencies testInputPaths;
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_TestInputPath,
                                              m_TestInputExcludePath,
                                              m_TestInputExcludedFiles,
                                              m_TestInputExcludePattern,
                                              m_TestInputPathRecurse,
                                              &m_TestInputPattern,
                                              "TestInputPath",
                                              testInputPaths ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }
    ASSERT( testInputPaths.GetSize() == m_TestInputPath.GetSize() ); // No need to store count since they should be the same

    // Store Static Dependencies
    m_StaticDependencies.SetCapacity( 1 + m_NumTestInputFiles + testInputPaths.GetSize() );
    m_StaticDependencies.Append( executable );
    m_StaticDependencies.Append( testInputFiles );
    m_StaticDependencies.Append( testInputPaths );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
TestNode::~TestNode() = default;

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool TestNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool UNUSED( forceClean ) )
{
    // clear dynamic deps from previous passes
    m_DynamicDependencies.Clear();

    // get the result of the directory lists and depend on those
    const size_t startIndex = 1 + m_NumTestInputFiles; // Skip Executable + TestInputFiles
    const size_t endIndex =  ( 1 + m_NumTestInputFiles + m_TestInputPath.GetSize() );
    for ( size_t i=startIndex; i<endIndex; ++i )
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
                FLOG_ERROR( "Test() .TestInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                return false;
            }

            m_DynamicDependencies.Append( Dependency( sn ) );
        }
        continue;
    }

    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TestNode::DoBuild( Job * job )
{
    Node::BuildResult result = NODE_RESULT_OK;  // first assume success

    AStackString<> workingDir;
    Array<AString> tmpFiles;
    if ( GetSandboxEnabled() )
    {
        // get worker sub dir in sandbox tmp
        WorkerThread::GetTempFileDirectory( workingDir );
        AStackString<> basePath( m_TestExecutable.Get(), m_TestExecutable.FindLast( NATIVE_SLASH ) );
        for ( const Dependency & dep : m_StaticDependencies )
        {
            Node* depNode = dep.GetNode();
            if ( depNode && depNode->IsAFile() )
            {
                const AString & depName = depNode->GetName();
                AStackString<> tmpFile;
                Node::GetSandboxTmpFile( basePath, depName, tmpFile );
                Node::EnsurePathExistsForFile( tmpFile );
                if ( FileIO::FileCopy( depName.Get(), tmpFile.Get() ) )
                {
                    // make dest file writable, so we can delete it below
                    const bool readOnly = false;
                    FileIO::SetReadOnly( tmpFile.Get(), readOnly );
                    tmpFiles.Append( tmpFile );
                }
            }
        }
    }
    else
    {
        workingDir = m_TestWorkingDir;
    }
    AStackString<> testExe( GetTestExecutable()->GetName() );

    EmitCompilationMessage( workingDir, testExe );

    // spawn the process
    Process p( FBuild::GetAbortBuildPointer() );
    const char * environmentString = ( FBuild::IsValid() ? FBuild::Get().GetEnvironmentString() : nullptr );

    AStackString<> spawnExe;
    bool spawnOK = false;
    if ( GetSandboxEnabled() )
    {
        AStackString<> spawnArgs;
        const AString & sandboxArgs = GetSandboxArgs();
        if ( !sandboxArgs.IsEmpty() )
        {
            spawnArgs += sandboxArgs;
            spawnArgs += ' ';
        }
        AStackString<> doubleQuote( "\"" );
        spawnArgs += doubleQuote;
        // use relative path, if we can; so we reduce command length
        AStackString<> testExeRelPath;
        PathUtils::GetPathGivenWorkingDir( workingDir, testExe, testExeRelPath );
        spawnArgs += testExeRelPath;
        spawnArgs += doubleQuote;
        spawnArgs += ' ';
        spawnArgs += m_TestArguments;
        spawnExe = GetAbsSandboxExe();
        // Spawn() expects nullptr, not empty string for working dir
        // have to use abs path for spawnExe
        spawnOK = p.Spawn( spawnExe.Get(),
            spawnArgs.Get(),
            workingDir.IsEmpty() ? nullptr : workingDir.Get(),
            environmentString );
    }
    else
    {
        spawnExe = testExe;
        // Spawn() expects nullptr, not empty string for working dir
        // have to use abs path for spawnExe
        spawnOK = p.Spawn( spawnExe.Get(),
            m_TestArguments.Get(),
            workingDir.IsEmpty() ? nullptr : workingDir.Get(),
            environmentString );
    }
    if ( spawnOK )
    {
        // capture all of the stdout and stderr
        AutoPtr< char > memOut;
        AutoPtr< char > memErr;
        uint32_t memOutSize = 0;
        uint32_t memErrSize = 0;
        bool timedOut = !p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize, m_TestTimeOut * 1000 );
        int exitStatus = p.WaitForExit();
        if ( !p.HasAborted() )
        {
            if ( ( timedOut == true ) || ( exitStatus != 0 ) || ( m_TestAlwaysShowOutput == true ) )
            {
                // print the test output
                Node::DumpOutput( job, memOut.Get(), memOutSize );
                Node::DumpOutput( job, memErr.Get(), memErrSize );
            }

            if ( timedOut == true )
            {
                FLOG_ERROR( "Test timed out after %u s (%s)", m_TestTimeOut, m_TestExecutable.Get() );
                result = NODE_RESULT_FAILED;
            }
            else if ( exitStatus != 0 )
            {
                    FLOG_ERROR( "Test failed Error: %s '%s'", ERROR_STR(exitStatus), m_TestExecutable.Get() );
                result = NODE_RESULT_FAILED;
            }

            // write the test output (saved for pass or fail)
            // no need to sandbox this file, since fbuild is writing it out to disk
            FileStream fs;
            if ( fs.Open( GetName().Get(), FileStream::WRITE_ONLY ) )
            {
                if ( ( memOut.Get() && ( fs.Write( memOut.Get(), memOutSize ) != memOutSize ) ) ||
                     ( memErr.Get() && ( fs.Write( memErr.Get(), memErrSize ) != memErrSize ) ) )
                {
                    AStackString<> outputName( GetName() );
                    // hide sandbox tmp dir in output; we want to keep the dir secret from other processes, since it is low integrity
                    HideSandboxTmpInString( outputName );
                    FLOG_ERROR( "Failed to write test output file '%s'", outputName.Get() );
                    result = NODE_RESULT_FAILED;
                }
                fs.Close();
            }
            else
            {
                AStackString<> outputName( GetName() );
                // hide sandbox tmp dir in output; we want to keep the dir secret from other processes, since it is low integrity
                HideSandboxTmpInString( outputName );
                FLOG_ERROR( "Failed to open test output file '%s'", outputName.Get() );
                result = NODE_RESULT_FAILED;
            }

            if ( ( timedOut == false ) && ( exitStatus == 0 ) )
            {
                // test passed
                // we only keep the "last modified" time of the test output for passed tests
                m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
            }
        }
        else
        {
            FLOG_ERROR( "Test unexpectedly aborted '%s'", m_TestExecutable.Get() );
            result = NODE_RESULT_FAILED;
        }
    }
    else  // !spawnOK
    {
        if ( p.HasAborted() )
        {
            FLOG_ERROR( "Test unexpectedly aborted '%s'", m_TestExecutable.Get() );
        }
        else if ( GetSandboxEnabled() )
        {
            AStackString<> outputName( GetName() );
            // hide sandbox tmp dir in output; we want to keep the dir secret from other processes, since it is low integrity
            HideSandboxTmpInString( outputName );
                FLOG_ERROR( "Failed to spawn '%s' process Error: %s to run '%s'\n",
                    spawnExe.Get(), LAST_ERROR_STR, outputName.Get() );
        }
        else
        {
            FLOG_ERROR( "Failed to spawn '%s' process Error: %s\n",
                spawnExe.Get(), LAST_ERROR_STR );
        }
        result = NODE_RESULT_FAILED;
    }
    for ( const AString & tmpFile : tmpFiles )
    {
        // delete the tmp files, so we don't grow disk space unbounded
        FileIO::FileDelete( tmpFile.Get() );
    }
    return result;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void TestNode::EmitCompilationMessage(
    const AString & workingDir, const AString & testExe ) const
{
    AStackString<> output;
    output += "Running Test: ";
    output += GetName();
    output += '\n';
    if ( FLog::ShowInfo() || ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandLines ) )
    {
        if ( GetSandboxEnabled() )
        {
            output += GetAbsSandboxExe();
            output += ' ';
            const AString & sandboxArgs = GetSandboxArgs();
            if ( !sandboxArgs.IsEmpty() )
            {
                output += sandboxArgs;
                output += ' ';
            }
        }
        output += testExe;
        output += ' ';
        output += m_TestArguments;
        output += '\n';
        if ( !workingDir.IsEmpty() )
        {
            output += "Working Dir: ";
            output += workingDir;
            output += '\n';
        }
        // hide sandbox tmp dir in any part of output (exe args and working dir)
        // we want to keep the dir secret from other processes, since it is low integrity
        HideSandboxTmpInString( output );
    }
    FLOG_BUILD_DIRECT( output.Get() );
}

//------------------------------------------------------------------------------
