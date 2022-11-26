// TestArgs.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

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

    // Helpers
    void Check( ArgsResponseFileMode mode,
                bool longArgs,
                bool expectedResult,
                bool expectResponseFileToBeUsed ) const;

    // Some constants for re-use between tests
    const AString mExeName{ "ExeName" };
    const AString mNodeName{ "NodeName" };

    void operator = (TestArgs&) = delete;
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
    //     Mode                         LongArgs    Success     UseResponseFile
    Check( ArgsResponseFileMode::NEVER, false,      true,       false );
}

// ResponseFile_Never_Long
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_Never_Long() const
{
    //     Mode                         LongArgs    Success     UseResponseFile
    Check( ArgsResponseFileMode::NEVER, true,       false,      false );
}

// ResponseFile_IfNeeded_Short
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_IfNeeded_Short() const
{
    //     Mode                             LongArgs    Success     UseResponseFile
    Check( ArgsResponseFileMode::IF_NEEDED, false,      true,       false );
}

// ResponseFile_IfNeeded_Long
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_IfNeeded_Long() const
{
    //     Mode                             LongArgs    Success     UseResponseFile
    Check( ArgsResponseFileMode::IF_NEEDED, true,       true,       true );
}

// ResponseFile_Always_Short
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_Always_Short() const
{
    //     Mode                             LongArgs    Success     UseResponseFile
    Check( ArgsResponseFileMode::ALWAYS,    false,      true,       true );
}

// ResponseFile_Always_Long
//------------------------------------------------------------------------------
void TestArgs::ResponseFile_Always_Long() const
{
    //     Mode                             LongArgs    Success     UseResponseFile
    Check( ArgsResponseFileMode::ALWAYS,    true,       true,       true );
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
        // Add ~3200 KiB of command line args ( 32 * 100 * 1024 )
        for ( size_t i = 0; i < 100 * 1024; ++i )
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
