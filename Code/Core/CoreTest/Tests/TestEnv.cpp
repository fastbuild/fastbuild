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

//------------------------------------------------------------------------------
TEST_GROUP( TestEnv, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestEnv, GetCommandLine )
{
    AStackString cmdLine;
    Env::GetCmdLine( cmdLine );
    TEST_ASSERT( cmdLine.FindI( "CoreTest" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestEnv, GetExePath )
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
TEST_CASE( TestEnv, GetCPUInfo )
{
    const CPUInfo & info = CPUInfo::Get();

    AStackString details;
    info.GetCPUDetailsString( details );
    OUTPUT( "CPU Info: %s\n", details.Get() );

    TEST_ASSERT( info.m_NumCores > 0 );
    TEST_ASSERT( info.m_NumCores == ( info.m_NumPCores + info.m_NumECores + info.m_NumLPECores ) );
}

//------------------------------------------------------------------------------
