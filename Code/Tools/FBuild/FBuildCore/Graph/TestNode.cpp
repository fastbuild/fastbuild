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
    REFLECT_ARRAY( m_TestInputExcludePath,    "TestInputExcludePath",     MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_TestInputExcludedFiles,  "TestInputExcludedFiles",   MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY( m_TestInputExcludePattern, "TestInputExcludePattern",  MetaOptional() )
    REFLECT(       m_TestArguments,           "TestArguments",        MetaOptional() )
    REFLECT(       m_TestWorkingDir,          "TestWorkingDir",       MetaOptional() + MetaPath() )
    REFLECT(       m_TestTimeOut,             "TestTimeOut",          MetaOptional() + MetaRange( 0, 4 * 60 * 60 ) ) // 4hrs
    REFLECT(       m_TestAlwaysShowOutput,    "TestAlwaysShowOutput", MetaOptional() )
    REFLECT_ARRAY( m_PreBuildDependencyNames, "PreBuildDependencies", MetaOptional() + MetaFile() + MetaAllowNonFile() )
    REFLECT_ARRAY( m_Environment,             "Environment",          MetaOptional() )
    REFLECT_ARRAY( m_RequiredWorkerTagStrings,   "RequiredWorkerTags",         MetaOptional() )

    // Internal State
    REFLECT(       m_NumTestInputFiles,          "NumTestInputFiles",          MetaHidden() )
REFLECT_END( TestNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
TestNode::TestNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
    , m_TestExecutable()
    , m_TestArguments()
    , m_TestWorkingDir()
    , m_TestTimeOut( 0 )
    , m_TestAlwaysShowOutput( false )
    , m_TestInputPathRecurse( true )
    , m_NumTestInputFiles( 0 )
    , m_EnvironmentString( nullptr )
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
TestNode::~TestNode()
{
    FREE( (void *)m_EnvironmentString );
}

// GetEnvironmentString
//------------------------------------------------------------------------------
const char * TestNode::GetEnvironmentString() const
{
    return Node::GetEnvironmentString( m_Environment, m_EnvironmentString );
}

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

// GetRequiredWorkerTags
//------------------------------------------------------------------------------
/*virtual*/ const Tags & TestNode::GetRequiredWorkerTags() const
{
    if ( m_RequiredWorkerTags.IsEmpty() )
    {
        // get the tags from the strings
        m_RequiredWorkerTags.ParseAndAddTags( m_RequiredWorkerTagStrings );
    }
    return m_RequiredWorkerTags;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TestNode::DoBuild( Job * job )
{
    Node::BuildResult result = NODE_RESULT_OK;  // first assume success

    // get the tags from the strings
    m_RequiredWorkerTags.ParseAndAddTags( m_RequiredWorkerTagStrings );

    if ( EnsureCanBuild( job ) )
    {
        AStackString<> workingDir;
        AStackString<> testExe;
        Array<AString> tmpFiles;
        workingDir = m_TestWorkingDir;
        testExe = GetTestExecutable()->GetName();

        EmitCompilationMessage( workingDir, testExe );

        // spawn the process
        Process p( FBuild::GetAbortBuildPointer() );
        const char * environmentString = GetEnvironmentString();

        AStackString<> spawnExe;
        bool spawnOK = false;
        spawnExe = testExe;
        // Spawn() expects nullptr, not empty string for working dir
        // have to use abs path for spawnExe
        spawnOK = p.Spawn( spawnExe.Get(),
            m_TestArguments.Get(),
            workingDir.IsEmpty() ? nullptr : workingDir.Get(),
            environmentString );
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
                FileStream fs;
                if ( fs.Open( GetName().Get(), FileStream::WRITE_ONLY ) )
                {
                    if ( ( memOut.Get() && ( fs.Write( memOut.Get(), memOutSize ) != memOutSize ) ) ||
                         ( memErr.Get() && ( fs.Write( memErr.Get(), memErrSize ) != memErrSize ) ) )
                    {
                        AStackString<> outputName( GetName() );
                        FLOG_ERROR( "Failed to write test output file '%s'", outputName.Get() );
                        result = NODE_RESULT_FAILED;
                    }
                    fs.Close();
                }
                else
                {
                    AStackString<> outputName( GetName() );
                    FLOG_ERROR( "Failed to open test output file '%s'", outputName.Get() );
                    result = NODE_RESULT_FAILED;
                }

                if ( ( timedOut == false ) && ( exitStatus == 0 ) )
                {
                    // test passed
                    // record new file time
                    RecordStampFromBuiltFile();
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
    }
    else
    {
        // EnsureCanBuild will have emitted an error
        result = NODE_RESULT_FAILED;
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
    }
    FLOG_BUILD_DIRECT( output.Get() );
}

//------------------------------------------------------------------------------
