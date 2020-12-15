// TestArgs.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
//#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
//#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
//#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThread.h"

//#include "Core/Strings/AStackString.h"

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
        // Add ~320 KiB of command line args ( 32 * 10 * 1024 )
        for ( size_t i = 0; i < 10 * 1024; ++i )
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
