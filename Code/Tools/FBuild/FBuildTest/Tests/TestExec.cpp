// TestExec.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Process/Process.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AStackString.h"

// TestExec
//------------------------------------------------------------------------------
class TestExec : public FBuildTest
{
private:
    DECLARE_TESTS

    void BuildHelperExe() const;
    void Build_ExecCommand_ExpectedSuccesses() const;
    void Build_ExecCommand_NoRebuild() const;
    void Build_ExecCommand_SingleInputChange() const;
    void Build_ExecCommand_MultipleInputChange() const;
    void Build_ExecCommand_UseStdOut() const;
    void Build_ExecCommand_ExpectedFailures() const;
    void Exclusions() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestExec )
    REGISTER_TEST( BuildHelperExe )
    REGISTER_TEST( Build_ExecCommand_ExpectedSuccesses )
    REGISTER_TEST( Build_ExecCommand_NoRebuild )
    REGISTER_TEST( Build_ExecCommand_SingleInputChange )
    REGISTER_TEST( Build_ExecCommand_MultipleInputChange )
    REGISTER_TEST( Build_ExecCommand_UseStdOut )
    REGISTER_TEST( Build_ExecCommand_ExpectedFailures )
    REGISTER_TEST( Exclusions )
REGISTER_TESTS_END

// Helpers
//------------------------------------------------------------------------------
void CreateInputFile( const AString & target )
{
    FileStream f;
    f.Open( target.Get(), FileStream::WRITE_ONLY );
    f.WriteBuffer( "I", 1 );
    f.Close();
}

// BuildResource
//------------------------------------------------------------------------------
void TestExec::BuildHelperExe() const
{
    // Make sure the helper executable will build properly

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExec/exec.bff";
    options.m_NumWorkerThreads = 1;

    FBuild fBuild( options );
    fBuild.Initialize();

    const AStackString<> exec( "../tmp/Test/Exec/exec.exe" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( exec );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "HelperExe" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Exec/exec.fdb" ) );

    // make sure all output is where it is expected
    EnsureFileExists( exec );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     1,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     1,      Node::EXE_NODE );
}

//------------------------------------------------------------------------------
void TestExec::Build_ExecCommand_ExpectedSuccesses() const
{
    // Build all the exec commands that are expected to be successes

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExec/exec.bff";

    FBuild fBuild( options );
    fBuild.Initialize( "../tmp/Test/Exec/exec.fdb" );

    // Make the relevant inputs
    const AStackString<> inFile_dummy( "../tmp/Test/Exec/dummy_file_does_not_exist.txt" );
    const AStackString<> inFile_oneInput( "../tmp/Test/Exec/OneInput.txt" );
    const AStackString<> inFile_stdout( "../tmp/Test/Exec/OneInput_StdOut.txt" );
    const AStackString<> inFile_multiInputA( "../tmp/Test/Exec/MultiInputA.txt" );
    const AStackString<> inFile_multiInputB( "../tmp/Test/Exec/MultiInputB.txt" );

    // First file commented out because it is supposed to not exist
    CreateInputFile( inFile_oneInput );
    CreateInputFile( inFile_stdout );
    CreateInputFile( inFile_multiInputA );
    CreateInputFile( inFile_multiInputB );

    // make sure all output is where it is expected
    const AStackString<> outFile_dummy( "../tmp/Test/Exec/dummy_file_does_not_exist.txt.out" );
    const AStackString<> outFile_oneInput( "../tmp/Test/Exec/OneInput.txt.out" );
    const AStackString<> outFile_stdout( "../tmp/Test/Exec/OneInput_StdOut.txt.stdout" );
    const AStackString<> outFile_multiInputA( "../tmp/Test/Exec/MultiInputA.txt.out" );
    const AStackString<> outFile_multiInputB( "../tmp/Test/Exec/MultiInputB.txt.out" );

    // clean up anything left over from previous runs
    EnsureFileDoesNotExist( outFile_dummy );
    EnsureFileDoesNotExist( outFile_oneInput );
    EnsureFileDoesNotExist( outFile_stdout );
    EnsureFileDoesNotExist( outFile_multiInputA );
    EnsureFileDoesNotExist( outFile_multiInputB );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "ExecCommandTest_ExpectedSuccesses" ) );
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/Exec/exec.fdb" ) );

    EnsureFileExists( outFile_dummy );
    EnsureFileExists( outFile_oneInput );
    EnsureFileExists( outFile_stdout );
    EnsureFileExists( outFile_multiInputA );
    EnsureFileExists( outFile_multiInputB );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     0,      Node::EXE_NODE );
    CheckStatsNode ( 4,     4,      Node::EXEC_NODE );
}

//------------------------------------------------------------------------------
void TestExec::Build_ExecCommand_NoRebuild() const
{
    // Rebuild the exec commands
    // - Only the command with a non-existant inputfile should rebuild

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExec/exec.bff";
    options.m_NumWorkerThreads = 1;

    FBuild fBuild( options );
    fBuild.Initialize( "../tmp/Test/Exec/exec.fdb" );

    TEST_ASSERT( fBuild.Build( "ExecCommandTest_ExpectedSuccesses" ) );

    // We expect only one command to run a second time (the one that always runs)

    // Check stats
    //               Seen,  Built,  Type
    // NOTE: Don't test file nodes since test used windows.h
    CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     0,      Node::EXE_NODE );
    CheckStatsNode ( 4,     1,      Node::EXEC_NODE );
}

//------------------------------------------------------------------------------
void TestExec::Build_ExecCommand_SingleInputChange() const
{
    // Rebuild one of the commands after a file has changed
    // - 1 execs should run this time (only asking one directly to run)

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExec/exec.bff";
    options.m_NumWorkerThreads = 1;

    FBuild fBuild( options );
    fBuild.Initialize( "../tmp/Test/Exec/exec.fdb" );

    const AStackString<> inFile_oneInput( "../tmp/Test/Exec/OneInput.txt" );
    #if defined( __OSX__ )
        // OS X FileSystem time granularity is poor, so we need to ensure enough time passes
        // so file is seen as modified
        Thread::Sleep( 1000 );
    #elif defined( __LINUX__ )
        Thread::Sleep( 1000 ); // Work around low time resolution of ext2/ext3/reiserfs and time caching used by used by others
    #endif
    CreateInputFile( inFile_oneInput );

    TEST_ASSERT( fBuild.Build( "ExecCommandTest_OneInput" ) );

    // We expect only one command to run a second time (the one that always runs)

    // Check stats
    //               Seen,  Built,  Type
    // NOTE: Don't test file nodes since test used windows.h
    CheckStatsNode ( 1,     0,      Node::OBJECT_NODE );
    CheckStatsNode ( 1,     0,      Node::OBJECT_LIST_NODE );
    CheckStatsNode ( 1,     1,      Node::ALIAS_NODE );
    CheckStatsNode ( 1,     0,      Node::EXE_NODE );
    CheckStatsNode ( 1,     1,      Node::EXEC_NODE );
}

//------------------------------------------------------------------------------
void TestExec::Build_ExecCommand_MultipleInputChange() const
{
    // Rebuild one of the commands after a file has changed
    // - 1 execs should run this time (only asking one directly to run)

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExec/exec.bff";
    options.m_NumWorkerThreads = 1;

    FBuild fBuild( options );
    fBuild.Initialize( "../tmp/Test/Exec/exec.fdb" );

    const AStackString<> inFile_multiInputA( "../tmp/Test/Exec/MultiInputA.txt" );
    CreateInputFile( inFile_multiInputA );

    TEST_ASSERT( fBuild.Build( "ExecCommandTest_MultipleInput" ) );

    // We expect only one command to run a second time (the one that always runs)

    // Check stats
    //               Seen,  Built,  Type
    // NOTE: Don't test file nodes since test used windows.h
    CheckStatsNode(1, 0, Node::OBJECT_NODE);
    CheckStatsNode(1, 0, Node::OBJECT_LIST_NODE);
    CheckStatsNode(1, 1, Node::ALIAS_NODE);
    CheckStatsNode(1, 0, Node::EXE_NODE);
    CheckStatsNode(1, 1, Node::EXEC_NODE);

    // ------- Now try the other file

    const AStackString<> inFile_multiInputB( "../tmp/Test/Exec/MultiInputB.txt" );
    CreateInputFile( inFile_multiInputB );

    TEST_ASSERT( fBuild.Build( "ExecCommandTest_MultipleInput" ) );

    // We expect only one command to run a second time (the one that always runs)

    // Check stats
    //               Seen,  Built,  Type
    // NOTE: Don't test file nodes since test used windows.h
    CheckStatsNode(1, 0, Node::OBJECT_NODE);
    CheckStatsNode(1, 0, Node::OBJECT_LIST_NODE);
    CheckStatsNode(1, 1, Node::ALIAS_NODE);
    CheckStatsNode(1, 0, Node::EXE_NODE);
    CheckStatsNode(1, 1, Node::EXEC_NODE);
}

//------------------------------------------------------------------------------
void TestExec::Build_ExecCommand_UseStdOut() const
{
    // Make sure the stdout from the executable
    // did actually make it into the stdout file

    const AStackString<> outFile_stdout( "../tmp/Test/Exec/OneInput_StdOut.txt.stdout" );
    EnsureFileExists( outFile_stdout );

    // Expected contents begin with:
    const AStackString<> expectedData( "Touched: " );
    const size_t firstLineBufferSize = 21;
    char firstLineBuffer[ firstLineBufferSize ];

    FileStream f;
    f.Open( outFile_stdout.Get(), FileStream::READ_ONLY );
    f.ReadBuffer( static_cast< char * >( &firstLineBuffer[ 0 ] ), firstLineBufferSize - 1 );
    f.Close();

    const AStackString<> firstLine( &firstLineBuffer[ 0 ], &firstLineBuffer[ firstLineBufferSize ] );
    TEST_ASSERT( firstLine.BeginsWith( expectedData ) );
}

//------------------------------------------------------------------------------
void TestExec::Build_ExecCommand_ExpectedFailures() const
{
    // Build all the exec commands that are expected to be failures
    // - Output file not getting written should error
    // - Expected return code not being output should error

    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExec/exec.bff";
    options.m_NumWorkerThreads = 2;

    FBuild fBuild( options );
    fBuild.Initialize( "../tmp/Test/Exec/exec.fdb" );

    // build
    Array< AString > targets( 2, false );
    targets.EmplaceBack( "ExecCommandTest_OneInput_ReturnCode_ExpectFail" );
    targets.EmplaceBack( "ExecCommandTest_OneInput_WrongOutput_ExpectFail" );
    TEST_ASSERT( !fBuild.Build( targets ) );
}

// Exclusions
//------------------------------------------------------------------------------
void TestExec::Exclusions() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestExec/Exclusions/fbuild.bff";
    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // build (via alias)
    TEST_ASSERT( fBuild.Build( "Test" ) );

    // Check all the exclusion methods worked as expected
    const char* const aliasesToCheck[] =
    {
        "ExcludePath-ForwardSlash",
        "ExcludePath-Backslash",
        "ExcludedFiles-File",
        "ExcludedFiles-Path-ForwardSlash",
        "ExcludedFiles-Path-Backslash",
        "ExcludePattern-ForwardSlash",
        "ExcludePattern-Backslash",
    };
    for (const char* const aliasToCheck : aliasesToCheck)
    {
        // Get the TestNode (via the Alias)
        const Node * aliasNode = fBuild.GetNode( aliasToCheck );
        TEST_ASSERT( aliasNode );
        const Node * testNode = aliasNode->GetStaticDependencies()[ 0 ].GetNode();
        TEST_ASSERT( testNode );

        // Check that it has one dynamic dependency, and that it's the 'B' file
        TEST_ASSERT( testNode->GetDynamicDependencies().GetSize() == 1 );
        TEST_ASSERT( testNode->GetDynamicDependencies()[ 0 ].GetNode()->GetName().EndsWithI( "FileB.txt" ) );
    }
}

//------------------------------------------------------------------------------
