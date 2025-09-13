// TestEnv.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include <Core/Env/Env.h>
#include <Core/Strings/AStackString.h>
#include <Core/Tracing/Tracing.h>

// TestEnv
//------------------------------------------------------------------------------
class TestEnv : public TestGroup
{
private:
    DECLARE_TESTS

    void GetCommandLine() const;
    void GetProcessorInfo() const;
    void GetExePath() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestEnv )
    REGISTER_TEST( GetCommandLine )
    REGISTER_TEST( GetProcessorInfo )
    REGISTER_TEST( GetExePath )
REGISTER_TESTS_END

// GetCommandLine
//------------------------------------------------------------------------------
void TestEnv::GetCommandLine() const
{
    AStackString cmdLine;
    Env::GetCmdLine( cmdLine );
    TEST_ASSERT( cmdLine.FindI( "CoreTest" ) );
}

// GetExePath
//------------------------------------------------------------------------------
void TestEnv::GetExePath() const
{
    AStackString cmdLine;
    Env::GetExePath( cmdLine );
#if defined( __WINDOWS__ )
    TEST_ASSERT( cmdLine.EndsWithI( "CoreTest.exe" ) );
#else
    TEST_ASSERT( cmdLine.EndsWithI( "CoreTest" ) );
#endif
}

// GetProcessorInfo
//------------------------------------------------------------------------------
void TestEnv::GetProcessorInfo() const
{
    const Env::ProcessorInfo & info = Env::GetProcessorInfo();
    OUTPUT( "Num Cores: %u (PCores: %u + ECores: %u)\n",
            info.mNumCores,
            info.mNumPCores,
            info.mNumECores );
}

//------------------------------------------------------------------------------
