// TestArgs.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

// Core
#include "Core/Process/Process.h"

// TestArgs
//------------------------------------------------------------------------------
class TestArgs : public FBuildTest
{
private:
    DECLARE_TESTS

    // Tests
    void Unused() const;
    void ResponseFile_Never_Short() const;
    void ResponseFile_Never_Long() const;
    void ResponseFile_IfNeeded_Short() const;
    void ResponseFile_IfNeeded_Long() const;
    void ResponseFile_Always_Short() const;
    void ResponseFile_Always_Long() const;
    void ResponseFile_CommandLineQuoting() const;
    void ArgumentParsing() const;

    // Helpers
    void Check( ArgsResponseFileMode mode,
                bool longArgs,
                bool expectedResult,
                bool expectResponseFileToBeUsed ) const;

    void CheckParsing( const char * commandLine,
                       const char * arg1 = nullptr,
                       const char * arg2 = nullptr ) const;

    // Some constants for re-use between tests
    const AString mExeName{ "ExeName" };
    const AString mNodeName{ "NodeName" };

    void operator=( TestArgs & ) = delete;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestArgs )
    REGISTER_TEST( ResponseFile_Never_Short )
    REGISTER_TEST( ResponseFile_Never_Long )
    REGISTER_TEST( ResponseFile_IfNeeded_Short )
    REGISTER_TEST( ResponseFile_IfNeeded_Long )
    REGISTER_TEST( ResponseFile_Always_Short )
    REGISTER_TEST( ResponseFile_Always_Long )
    REGISTER_TEST( ResponseFile_CommandLineQuoting )
    REGISTER_TEST( ArgumentParsing )
REGISTER_TESTS_END

// Unused
//------------------------------------------------------------------------------
void TestArgs::Unused() const
{
    Args args;
}

// ResponseFile_Never_Short
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_Never_Short() const
{
    //     Mode                         LongArgs, Success, UseResponseFile
    Check( ArgsResponseFileMode::NEVER, false, true, false );
}

// ResponseFile_Never_Long
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_Never_Long() const
{
    //     Mode                         LongArgs, Success, UseResponseFile
    Check( ArgsResponseFileMode::NEVER, true, false, false );
}

// ResponseFile_IfNeeded_Short
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_IfNeeded_Short() const
{
    //     Mode                             LongArgs, Success, UseResponseFile
    Check( ArgsResponseFileMode::IF_NEEDED, false, true, false );
}

// ResponseFile_IfNeeded_Long
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_IfNeeded_Long() const
{
    //     Mode                             LongArgs, Success, UseResponseFile
    Check( ArgsResponseFileMode::IF_NEEDED, true, true, true );
}

// ResponseFile_Always_Short
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_Always_Short() const
{
    //     Mode                             LongArgs, Success, UseResponseFile
    Check( ArgsResponseFileMode::ALWAYS, false, true, true );
}

// ResponseFile_Always_Long
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_Always_Long() const
{
    //     Mode                             LongArgs, Success, UseResponseFile
    Check( ArgsResponseFileMode::ALWAYS, true, true, true );
}

// ResponseFile_CommandLineQuoting
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_CommandLineQuoting() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestArgs/ResponseFile/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "ResponseFilePath" ) );

    // Ensure response file use is forced on the compiler
    StackArray<const Node *> nodes;
    fBuild.GetNodesOfType( Node::COMPILER_NODE, nodes );
    TEST_ASSERT( nodes.IsEmpty() == false );
    for ( const Node * node : nodes )
    {
        const CompilerNode * cn = (const CompilerNode *)node;
        TEST_ASSERT( cn->ShouldForceResponseFileUse() == true );
    }
}

// Check
//------------------------------------------------------------------------------
void TestArgs::Check( ArgsResponseFileMode mode,
                      bool longArgs,
                      bool expectedResult,
                      bool expectResponseFileToBeUsed ) const
{
    // Build args
    Args args;

    // Prevent actual write to disk as this depends on running in a WorkerThread
    args.DisableResponseFileWrite();

    if ( longArgs )
    {
        // Add ~32 MiB of command line args ( 32 * 1024 * 1024 )
        for ( size_t i = 0; i < 1024 * 1024; ++i )
        {
            args += "123456789012345678901234567890X"; // 31 chars
            args.AddDelimiter();
        }
    }
    else
    {
        // Add short args
        args += "short";
    }

    // Check result
    if ( expectedResult == true )
    {
        // Finalize should succeed
        TEST_ASSERT( args.Finalize( mExeName, mNodeName, mode ) == true );
    }
    else
    {
        // Finalize should fail
        TEST_ASSERT( args.Finalize( mExeName, mNodeName, mode ) == false );

        // Output should contain error
        TEST_ASSERT( GetRecordedOutput().Find( "Command Line Limit Exceeded" ) );
    }

    // Check if response file was used
    const bool responseFileWasUsed = ( args.GetFinalArgs().Find( '@' ) != nullptr );
    TEST_ASSERT( responseFileWasUsed == expectResponseFileToBeUsed );
}

//------------------------------------------------------------------------------
void TestArgs::ArgumentParsing() const
{
    // Build executable we'll invoke
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestArgs/ResponseFile/ArgumentParsing/fbuild.bff";

    FBuildForTest fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );
    TEST_ASSERT( fBuild.Build( "Exe" ) );

    // Empty
    CheckParsing( "" );

    // Multiple args
    CheckParsing( "-aaa -bbb", "-aaa", "-bbb" );

    // Quoted args
    CheckParsing( R"(-aaa "-DARG")", "-aaa", "-DARG" );
    CheckParsing( R"(-aaa -D"ARG")", "-aaa", "-DARG" );

    // Escaped quotes
    CheckParsing( R"(-aaa -D\"ARG\")", "-aaa", R"(-D"ARG")" );
    CheckParsing( R"(-aaa "-D\"ARG\"")", "-aaa", R"(-D"ARG")" );

    // Spaces inside quotes
    CheckParsing( R"("-DTHING=\"   \"")", R"(-DTHING="   ")" );
}

// CheckParsing
//------------------------------------------------------------------------------
void TestArgs::CheckParsing( const char * commandLine,
                             const char * arg1,
                             const char * arg2 ) const
{
    // Invoke previously built exe which echoes back commands
    Process p;
    TEST_ASSERT( p.Spawn( "../tmp/Test/Args/ResponseFile/ArgumentParsing/Exe.exe",
                          commandLine,
                          nullptr, // working dir
                          nullptr ) ); // environment

    // Capture the output
    AStackString out;
    AStackString err;
    TEST_ASSERT( p.ReadAllData( out, err ) );
    TEST_ASSERT( p.WaitForExit() == 0 );

    // Normalize windows line endings
#if defined( __WINDOWS__ )
    out.Replace( "\r\n", "\n" );
#endif

    // Generate expected results
    AStackString expected;
    const size_t numArgsExpected = ( arg1 ? 1u : 0u ) +
                                   ( arg2 ? 1u : 0u );
    expected.Format( "%zu\n", numArgsExpected );
    if ( arg1 )
    {
        expected.AppendFormat( "%s\n", arg1 );
    }
    if ( arg2 )
    {
        expected.AppendFormat( "%s\n", arg2 );
    }

    // Compare echoed result with what we expect
    TEST_ASSERT( out == expected );
}

//------------------------------------------------------------------------------
