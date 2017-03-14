// TestNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "TestNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Process.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( TestNode, Node, MetaName( "TestOutput" ) + MetaFile() )
    REFLECT( m_TestExecutable,      "TestExecutable",       MetaFile() )
    REFLECT( m_TestArguments,       "TestArguments",        MetaOptional() )
    REFLECT( m_TestWorkingDir,      "TestWorkingDir",       MetaOptional() + MetaPath() )
    REFLECT( m_TestTimeOut,         "TestTimeOut",          MetaOptional() + MetaRange( 0, 4 * 60 * 60 ) ) // 4hrs
    REFLECT( m_TestAlwaysShowOutput,"TestAlwaysShowOutput", MetaOptional() )
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
{
    m_Type = Node::TEST_NODE;
}

// Initialize
//------------------------------------------------------------------------------
bool TestNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // Get node for Executable
    if ( !function->GetFileNode( nodeGraph, iter, m_TestExecutable, "TestExecutable", m_StaticDependencies ) )
    {
        return false; // GetFileNode will have emitted an error
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
TestNode::~TestNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TestNode::DoBuild( Job * job )
{
    // If the workingDir is empty, use the current dir for the process
    const char * workingDir = m_TestWorkingDir.IsEmpty() ? nullptr : m_TestWorkingDir.Get();

    EmitCompilationMessage( workingDir );

    // spawn the process
    Process p;
    bool spawnOK = p.Spawn( GetTestExecutable()->GetName().Get(),
                            m_TestArguments.Get(),
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
    bool timedOut = !p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize, m_TestTimeOut * 1000 );
    if ( timedOut )
    {
        FLOG_ERROR( "Test timed out after %u s (%s)", m_TestTimeOut, m_TestExecutable.Get() );
        return NODE_RESULT_FAILED;
    }

    ASSERT( !p.IsRunning() );
    // Get result
    int result = p.WaitForExit();
    if ( ( result != 0 ) || ( m_TestAlwaysShowOutput == true ) )
    {
        // something went wrong, print details
        Node::DumpOutput( job, memOut.Get(), memOutSize );
        Node::DumpOutput( job, memErr.Get(), memErrSize );
    }

    // write the test output (saved for pass or fail)
    FileStream fs;
    if ( fs.Open( GetName().Get(), FileStream::WRITE_ONLY ) == false )
    {
        FLOG_ERROR( "Failed to open test output file '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }
    if ( ( memOut.Get() && ( fs.Write( memOut.Get(), memOutSize ) != memOutSize ) ) ||
         ( memErr.Get() && ( fs.Write( memErr.Get(), memErrSize ) != memErrSize ) ) )
    {
        FLOG_ERROR( "Failed to write test output file '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }
    fs.Close();

    // did the test fail?
    if ( result != 0 )
    {
        FLOG_ERROR( "Test failed (error %i) '%s'", result, GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // test passed
    // we only keep the "last modified" time of the test output for passed tests
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    return NODE_RESULT_OK;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void TestNode::EmitCompilationMessage( const char * workingDir ) const
{
    AStackString<> output;
    output += "Running Test: ";
    output += GetName();
    output += '\n';
    if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
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
    FLOG_BUILD_DIRECT( output.Get() );
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void TestNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * TestNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    TestNode * node = nodeGraph.CreateTestNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }
    return node;
}

//------------------------------------------------------------------------------
