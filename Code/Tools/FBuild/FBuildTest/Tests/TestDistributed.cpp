// TestDistributed.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// TestDistributed
//------------------------------------------------------------------------------
class TestDistributed : public UnitTest
{
private:
    DECLARE_TESTS

    void TestWith1RemoteWorkerThread() const;
    void TestWith4RemoteWorkerThreads() const;
    void WithPCH() const;
    void RegressionTest_RemoteCrashOnErrorFormatting();
    void TestLocalRace();
    void AnonymousNamespaces();
    void TestForceInclude() const;
    void TestZiDebugFormat() const;
    void TestZiDebugFormat_Local() const;
    void D8049_ToolLongDebugRecord() const;

    void TestHelper( const char * target,
                     uint32_t numRemoteWorkers,
                     bool shouldFail = false,
                     bool allowRace = false ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestDistributed )
    REGISTER_TEST( TestWith1RemoteWorkerThread )
    REGISTER_TEST( TestWith4RemoteWorkerThreads )
    REGISTER_TEST( WithPCH )
    REGISTER_TEST( RegressionTest_RemoteCrashOnErrorFormatting )
    REGISTER_TEST( TestLocalRace )
    REGISTER_TEST( AnonymousNamespaces )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( TestForceInclude )
        REGISTER_TEST( TestZiDebugFormat )
        REGISTER_TEST( TestZiDebugFormat_Local )
        REGISTER_TEST( D8049_ToolLongDebugRecord )
    #endif
REGISTER_TESTS_END

// Test
//------------------------------------------------------------------------------
void TestDistributed::TestHelper( const char * target, uint32_t numRemoteWorkers, bool shouldFail, bool allowRace ) const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = allowRace;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    JobQueueRemote jqr( numRemoteWorkers );

    // start a client to emulate the other end
    Server s;
    s.Listen( Protocol::PROTOCOL_PORT );

    // clean up anything left over from previous runs
    Array< AString > files;
    FileIO::GetFiles( AStackString<>( "../../../../tmp/Test/Distributed" ), AStackString<>( "*.*" ), true, &files );
    const AString * iter = files.Begin();
    const AString * const end = files.End();
    for ( ; iter != end; ++iter )
    {
        FileIO::FileDelete( iter->Get() );
    }

    if ( !shouldFail )
    {
        TEST_ASSERT( FileIO::FileExists( target ) == false );
    }

    bool pass = fBuild.Build( AStackString<>( target ) );
    if ( !shouldFail )
    {
        TEST_ASSERT( pass );
    }

    // make sure all output files are as expected
    if ( !shouldFail )
    {
        TEST_ASSERT( FileIO::FileExists( target ) );
    }
}

// TestWith1RemoteWorkerThread
//------------------------------------------------------------------------------
void TestDistributed::TestWith1RemoteWorkerThread() const
{
    const char * target( "../../../../tmp/Test/Distributed/dist.lib" );
    TestHelper( target, 1 );
}

// TestWith4RemoteWorkerThreads
//------------------------------------------------------------------------------
void TestDistributed::TestWith4RemoteWorkerThreads() const
{
    const char * target( "../../../../tmp/Test/Distributed/dist.lib" );
    TestHelper( target, 4 );
}

// WithPCH
//------------------------------------------------------------------------------
void TestDistributed::WithPCH() const
{
    const char * target( "../../../../tmp/Test/Distributed/distpch.lib" );
    TestHelper( target, 4 );
}

// RegressionTest_RemoteCrashOnErrorFormatting
//------------------------------------------------------------------------------
void TestDistributed::RegressionTest_RemoteCrashOnErrorFormatting()
{
    const char * target( "badcode" );
    TestHelper( target, 4, true ); // compilation should fail
}

//------------------------------------------------------------------------------
void TestDistributed::TestLocalRace()
{
    {
        const char * target( "../../../../tmp/Test/Distributed/dist.lib" );
        TestHelper( target, 1, false, true ); // allow race
    }
    {
        const char * target( "../../../../tmp/Test/Distributed/dist.lib" );
        TestHelper( target, 4, false, true ); // allow race
    }
    {
        const char * target( "../../../../tmp/Test/Distributed/distpch.lib" );
        TestHelper( target, 4, false, true ); // allow race
    }
    {
        const char * target( "badcode" );
        TestHelper( target, 4, true, true ); // compilation should fail, allow race
    }
}

// AnonymousNamespaces
//------------------------------------------------------------------------------
void TestDistributed::AnonymousNamespaces()
{
    // Check that compiling multiple objects with identically named symbols
    // in anonymouse namespaces don't cause link errors.  This is because
    // the MS compiler uses the path to the cpp file being compiled to
    // generate the symbol name (it doesn't respect the #line directives)
    const char * target( "../../../../tmp/Test/Distributed/AnonymousNamespaces/AnonymousNamespaces.lib" );
    TestHelper( target, 1 );
}

// TestForceInclude
//------------------------------------------------------------------------------
void TestDistributed::TestForceInclude() const
{
    const char * target( "../../../../tmp/Test/Distributed/ForceInclude/ForceInclude.lib" );
    TestHelper( target, 4 );
}

// TestZiDebugFormat
//------------------------------------------------------------------------------
void TestDistributed::TestZiDebugFormat() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    JobQueueRemote jqr( 1 );

    // start a client to emulate the other end
    Server s;
    s.Listen( Protocol::PROTOCOL_PORT );

    TEST_ASSERT( fBuild.Build( AStackString<>( "remoteZi" ) ) );
}

// TestZiDebugFormat_Local
//------------------------------------------------------------------------------
void TestDistributed::TestZiDebugFormat_Local() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    JobQueueRemote jqr( 1 );

    // start a client to emulate the other end
    Server s;
    s.Listen( Protocol::PROTOCOL_PORT );

    TEST_ASSERT( fBuild.Build( AStackString<>( "remoteZi" ) ) );
}

// D8049_ToolLongDebugRecord
//------------------------------------------------------------------------------
void TestDistributed::D8049_ToolLongDebugRecord() const
{
    FBuildOptions options;
    options.m_ConfigFile = "Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    JobQueueRemote jqr( 1 );

    // start a client to emulate the other end
    Server s;
    s.Listen( Protocol::PROTOCOL_PORT );

    TEST_ASSERT( fBuild.Build( AStackString<>( "D8049" ) ) );
}

//------------------------------------------------------------------------------
