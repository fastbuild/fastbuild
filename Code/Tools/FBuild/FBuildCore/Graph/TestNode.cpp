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
    REFLECT_ARRAY( m_ExtraFiles,       "ExtraFiles",          MetaOptional() + MetaFile() )
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
    REFLECT(       m_AllowDistribution,       "AllowDistribution",    MetaOptional() )
    REFLECT(       m_ExecutableRootPath,      "ExecutableRootPath",   MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_CustomEnvironmentVariables, "CustomEnvironmentVariables", MetaOptional() )
    REFLECT(       m_DeleteRemoteFilesWhenDone,  "DeleteRemoteFilesWhenDone",  MetaOptional() )

    // Internal State
    REFLECT(       m_NumTestInputFiles, "NumTestInputFiles",      MetaHidden() )
    REFLECT_STRUCT( m_Manifest,         "Manifest", ToolManifest, MetaHidden() + MetaIgnoreForComparison() )
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
    , m_AllowDistribution( false )
    , m_ExecutableRootPath()
    , m_DeleteRemoteFilesWhenDone( true )
    , m_TestInputPathRecurse( true )
    , m_NumTestInputFiles ( 0 )
    , m_EnvironmentString( nullptr )
{
    m_Type = Node::TEST_NODE;
}

// CONSTRUCTOR (Remote)
//------------------------------------------------------------------------------
TestNode::TestNode( const AString & objectName, const AString & testExecutable,
    const AString & testArguments, const AString & testWorkingDir, const uint32_t testTimeOut,
    const bool testAlwaysShowOutput, const bool allowDistribution,
    const Array< AString > & customEnvironmentVariables )
: FileNode( objectName, Node::FLAG_NONE )
    , m_TestExecutable(testExecutable)
    , m_TestArguments(testArguments)
    , m_TestWorkingDir(testWorkingDir)
    , m_TestTimeOut(testTimeOut)
    , m_TestAlwaysShowOutput(testAlwaysShowOutput)
    , m_AllowDistribution( allowDistribution )
    , m_ExecutableRootPath()
    , m_CustomEnvironmentVariables( customEnvironmentVariables )
    , m_DeleteRemoteFilesWhenDone( true )  // ignored, manifest uses its own value
    , m_TestInputPathRecurse( true )
    , m_NumTestInputFiles ( 0 )
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

    if( m_ExecutableRootPath.IsEmpty() )
    {
        const char * lastSlash = m_TestExecutable.FindLast( NATIVE_SLASH );
        if ( lastSlash )
        {
            m_ExecutableRootPath.Assign( m_TestExecutable.Get(), lastSlash );
        }
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

    // .ExtraFiles
    Dependencies extraFiles( 32, true );
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".ExtraFiles", m_ExtraFiles, extraFiles ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    // Check for conflicting files
    AStackString<> relPathExe;
    ToolManifest::GetRelativePath( m_ExecutableRootPath, m_TestExecutable, relPathExe );

    const size_t numExtraFiles = extraFiles.GetSize();
    for ( size_t i=0; i<numExtraFiles; ++i )
    {
        AStackString<> relPathA;
        ToolManifest::GetRelativePath( m_ExecutableRootPath, extraFiles[ i ].GetNode()->GetName(), relPathA );

        // Conflicts with Exe?
        if ( PathUtils::ArePathsEqual( relPathA, relPathExe ) )
        {
            Error::Error_1100_AlreadyDefined( iter, function, relPathA );
            return false;
        }

        // Conflicts with another file?
        for ( size_t j=(i+1); j<numExtraFiles; ++j )
        {
            AStackString<> relPathB;
            ToolManifest::GetRelativePath( m_ExecutableRootPath, extraFiles[ j ].GetNode()->GetName(), relPathB );

            if ( PathUtils::ArePathsEqual( relPathA, relPathB ) )
            {
                Error::Error_1100_AlreadyDefined( iter, function, relPathA );
                return false;
            }
        }
    }

    // Store Static Dependencies
    m_StaticDependencies.SetCapacity( 1 + m_NumTestInputFiles + testInputPaths.GetSize() + extraFiles.GetSize() );
    m_StaticDependencies.Append( executable );
    m_StaticDependencies.Append( testInputFiles );
    m_StaticDependencies.Append( testInputPaths );
    m_StaticDependencies.Append( extraFiles );

    m_Manifest.Initialize(
        m_ExecutableRootPath, m_StaticDependencies,
        m_DeleteRemoteFilesWhenDone );

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

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TestNode::DoBuild( Job * job )
{
    if ( !m_Manifest.DoBuild( m_StaticDependencies ) )
    {
        return NODE_RESULT_FAILED; // DoBuild will have emitted error
    }

    const bool canDistribute = m_AllowDistribution && FBuild::Get().GetOptions().m_AllowDistributed;
    if (canDistribute)
    {
        // yes... re-queue for secondary build
        return NODE_RESULT_NEED_SECOND_BUILD_PASS;
    }
    else
    {
        // can't do the work remotely, so try to do it locally right now
        bool stealingRemoteJob = false;
        bool racingRemoteJob = false;
        return DoBuildCommon(job, stealingRemoteJob, racingRemoteJob);
    }
}

// DoBuild_Remote
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TestNode::DoBuild2( Job * job, bool racingRemoteJob = false )
{
    bool stealingRemoteJob = job->IsLocal(); // are we stealing a remote job?
    return DoBuildCommon( job, stealingRemoteJob, racingRemoteJob);
}

// DoBuildCommon
//------------------------------------------------------------------------------
Node::BuildResult TestNode::DoBuildCommon( Job * job,
    bool stealingRemoteJob, bool racingRemoteJob)
{
    Node::BuildResult result = NODE_RESULT_OK;  // first assume success
    const bool isRemote = ( job->IsLocal() == false );

    // Use the remotely synchronized exe if building remotely
    AStackString<> workingDir;
    AStackString<> testExe;
    Array<AString> tmpFiles;
    if ( !isRemote )
    {
        workingDir = m_TestWorkingDir;
        testExe = GetTestExecutable()->GetName();
    }
    else
    {
        ASSERT( job->GetToolManifest() );
        job->GetToolManifest()->GetRemotePath( workingDir );
        job->GetToolManifest()->GetRemoteFilePath( 0, testExe );
    }

    EmitCompilationMessage( stealingRemoteJob, racingRemoteJob, isRemote, workingDir, testExe );

    // spawn the process
    Process p( FBuild::GetAbortBuildPointer() );
    const char * environmentString = nullptr;
    const char * ownedEnvironmentString = nullptr;
    if ( ( job->IsLocal() == false ) && ( job->GetToolManifest() ) )
    {
        environmentString =
            job->GetToolManifest()->GetRemoteEnvironmentString(
                m_CustomEnvironmentVariables,
                ownedEnvironmentString );
    }
    else
    {
        environmentString = GetEnvironmentString();
    }

    AStackString<> spawnExe;
    bool spawnOK = false;
    spawnExe = testExe;
    // Spawn() expects nullptr, not empty string for working dir
    // have to use abs path for spawnExe
    spawnOK = p.Spawn( spawnExe.Get(),
        m_TestArguments.Get(),
        workingDir.IsEmpty() ? nullptr : workingDir.Get(),
        environmentString );

    FREE( (void *)ownedEnvironmentString );
    ownedEnvironmentString = nullptr;
    environmentString = nullptr;

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
                m_Stamp = Math::Max( m_Stamp, m_Manifest.GetTimeStamp() );
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
    return result;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void TestNode::EmitCompilationMessage(
    bool stealingRemoteJob, bool racingRemoteJob, bool isRemote,
    const AString & workingDir, const AString & testExe ) const
{
    AStackString<> output;
    output += "Running Test: ";
    output += GetName();
    if ( racingRemoteJob )
    {
        output += " <LOCAL RACE>";
    }
    else if ( stealingRemoteJob )
    {
        output += " <LOCAL>";
    }
    output += '\n';
    if ( FLog::ShowInfo() || ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandLines ) || isRemote )
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

// SaveRemote
//------------------------------------------------------------------------------
/*virtual*/ void TestNode::SaveRemote( IOStream & stream ) const
{
    // Save minimal information for the remote worker
    stream.Write( m_Name );
    stream.Write( m_TestExecutable );
    stream.Write( m_TestArguments );
    stream.Write( m_TestWorkingDir );
    stream.Write( m_TestTimeOut );
    stream.Write( m_TestAlwaysShowOutput );
    stream.Write( m_AllowDistribution );
    stream.Write( m_CustomEnvironmentVariables );
    
    // don't save m_DeleteRemoteFilesWhenDone, since manifest uses its own value
}

// LoadRemote
//------------------------------------------------------------------------------
/*static*/ Node * TestNode::LoadRemote( IOStream & stream )
{
    AStackString<> name; 
    AStackString<> testExecutable; 
    AStackString<> testArguments; 
    AStackString<> testWorkingDir; 
    uint32_t testTimeOut = 0; 
    bool testAlwaysShowOutput = false; 
    bool allowDistribution = false; 
    Array< AString > customEnvironmentVariables;
    if ( ( stream.Read( name ) == false ) ||
         ( stream.Read( testExecutable ) == false ) ||
         ( stream.Read( testArguments ) == false ) ||
         ( stream.Read( testWorkingDir ) == false ) ||
         ( stream.Read( testTimeOut ) == false ) ||
         ( stream.Read( testAlwaysShowOutput ) == false ) ||
         ( stream.Read( allowDistribution ) == false ) ||
         ( stream.Read( customEnvironmentVariables ) == false ) )
    {
        return nullptr; 
    }

    return FNEW( TestNode( name, testExecutable, testArguments,
        testWorkingDir, testTimeOut, testAlwaysShowOutput,
        allowDistribution, customEnvironmentVariables ) );
}

// Migrate
//------------------------------------------------------------------------------
/*virtual*/ void TestNode::Migrate( const Node & oldNode )
{
    // Migrate Node level properties
    Node::Migrate( oldNode );

    // Migrate the timestamp/hash info stored for the files in the ToolManifest
    m_Manifest.Migrate( oldNode.CastTo<TestNode>()->GetManifest() );
}

//------------------------------------------------------------------------------
