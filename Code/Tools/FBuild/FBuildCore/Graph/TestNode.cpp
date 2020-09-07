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

#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Process.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( TestNode, Node, MetaName( "TestOutput" ) + MetaFile() )
    REFLECT(        m_TestExecutable,           "TestExecutable",           MetaFile() )
    REFLECT_ARRAY(  m_TestInput,                "TestInput",                MetaOptional() + MetaFile() )
    REFLECT_ARRAY(  m_TestInputPath,            "TestInputPath",            MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_TestInputPattern,         "TestInputPattern",         MetaOptional() )
    REFLECT(        m_TestInputPathRecurse,     "TestInputPathRecurse",     MetaOptional() )
    REFLECT_ARRAY(  m_TestInputExcludePath,     "TestInputExcludePath",     MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_TestInputExcludedFiles,   "TestInputExcludedFiles",   MetaOptional() + MetaFile( true ) )
    REFLECT_ARRAY(  m_TestInputExcludePattern,  "TestInputExcludePattern",  MetaOptional() + MetaFile( true ) )
    REFLECT(        m_TestArguments,            "TestArguments",            MetaOptional() )
    REFLECT(        m_TestWorkingDir,           "TestWorkingDir",           MetaOptional() + MetaPath() )
    REFLECT(        m_TestTimeOut,              "TestTimeOut",              MetaOptional() + MetaRange( 0, 4 * 60 * 60 ) ) // 4hrs
    REFLECT(        m_TestAlwaysShowOutput,     "TestAlwaysShowOutput",     MetaOptional() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,  "PreBuildDependencies",     MetaOptional() + MetaFile() + MetaAllowNonFile() )
    REFLECT_ARRAY(  m_Environment,              "Environment",              MetaOptional() )

    // Internal State
    REFLECT(        m_NumTestInputFiles,        "NumTestInputFiles",        MetaHidden() )
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
/*virtual*/ bool TestNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
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
                                              false, // Don't include read-only status in hash
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
/*virtual*/ bool TestNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool /*forceClean*/ )
{
    // clear dynamic deps from previous passes
    m_DynamicDependencies.Clear();

    // get the result of the directory lists and depend on those
    const size_t startIndex = 1 + m_NumTestInputFiles; // Skip Executable + TestInputFiles
    const size_t endIndex =  ( 1 + m_NumTestInputFiles + m_TestInputPath.GetSize() );
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
                FLOG_ERROR( "Test() .TestInputFile '%s' is not a FileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
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
/*virtual*/ Node::BuildResult TestNode::DoBuild( Job * job )
{
    // If the workingDir is empty, use the current dir for the process
    const char * workingDir = m_TestWorkingDir.IsEmpty() ? nullptr : m_TestWorkingDir.Get();

    EmitCompilationMessage( workingDir );

    // spawn the process
    Process p( FBuild::Get().GetAbortBuildPointer() );
    const char * environmentString = GetEnvironmentString();

    bool spawnOK = p.Spawn( GetTestExecutable()->GetName().Get(),
                            m_TestArguments.Get(),
                            workingDir,
                            environmentString );

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
    AString memOut;
    AString memErr;
    bool timedOut = !p.ReadAllData( memOut, memErr, m_TestTimeOut * 1000 );

    // Get result
    int result = p.WaitForExit();
    if ( p.HasAborted() )
    {
        return NODE_RESULT_FAILED;
    }

    if ( ( timedOut == true ) || ( result != 0 ) || ( m_TestAlwaysShowOutput == true ) )
    {
        // something went wrong, print details
        Node::DumpOutput( job, memOut );
        Node::DumpOutput( job, memErr );
    }

    if ( timedOut == true )
    {
        FLOG_ERROR( "Test timed out after %u s (%s)", m_TestTimeOut, m_TestExecutable.Get() );
    }
    else if ( result != 0 )
    {
        FLOG_ERROR( "Test failed. Error: %s Target: '%s'", ERROR_STR( result ), GetName().Get() );
    }

    // write the test output (saved for pass or fail)
    FileStream fs;
    if ( fs.Open( GetName().Get(), FileStream::WRITE_ONLY ) == false )
    {
        FLOG_ERROR( "Failed to open test output file '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }
    if ( ( ( memOut.IsEmpty() == false ) && ( fs.Write( memOut.Get(), memOut.GetLength() ) != memOut.GetLength() ) ) ||
         ( ( memErr.IsEmpty() == false ) && ( fs.Write( memErr.Get(), memErr.GetLength() ) != memErr.GetLength() ) ) )
    {
        FLOG_ERROR( "Failed to write test output file '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }
    fs.Close();

    // did the test fail?
    if ( ( timedOut == true ) || ( result != 0 ) )
    {
        return NODE_RESULT_FAILED;
    }

    // test passed

    // record new file time
    RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void TestNode::EmitCompilationMessage( const char * workingDir ) const
{
    AStackString<> output;
    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        output += "Running Test: ";
        output += GetName();
        output += '\n';
    }
    if ( FBuild::Get().GetOptions().m_ShowCommandLines )
    {
        output += GetTestExecutable()->GetName();
        output += ' ';
        output += m_TestArguments;
        output += '\n';
        if ( workingDir )
        {
            output += "Working Dir: ";
            output += workingDir;
            output += '\n';
        }
    }
    FLOG_OUTPUT( output );
}

//------------------------------------------------------------------------------
