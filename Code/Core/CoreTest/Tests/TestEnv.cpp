// TestEnv.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/Env/CPUInfo.h"
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
    void GetCPUInfo() const;
    void GetExePath() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestEnv )
    REGISTER_TEST( GetCommandLine )
    REGISTER_TEST( GetCPUInfo )
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

//------------------------------------------------------------------------------
void TestEnv::GetCPUInfo() const
{
    const CPUInfo & info = CPUInfo::Get();

    AStackString details;
    info.GetCPUDetailsString( details );
    OUTPUT( "CPU Info: %s\n", details.Get() );

    TEST_ASSERT( info.m_NumCores > 0 );
    TEST_ASSERT( info.m_NumCores == ( info.m_NumPCores + info.m_NumECores + info.m_NumLPECores ) );
}

//------------------------------------------------------------------------------
