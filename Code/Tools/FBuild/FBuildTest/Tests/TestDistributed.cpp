// TestDistributed.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// Defines
//------------------------------------------------------------------------------
#define TEST_PROTOCOL_PORT ( Protocol::PROTOCOL_PORT + 1 ) // Avoid conflict with real worker

// TestDistributed
//------------------------------------------------------------------------------
class TestDistributed : public FBuildTest
{
private:
    DECLARE_TESTS

    void TestWith1RemoteWorkerThread() const;
    void TestWith4RemoteWorkerThreads() const;
    void WithPCH() const;
    void RegressionTest_RemoteCrashOnErrorFormatting();
    void TestLocalRace();
    void RemoteRaceWinRemote();
    void AnonymousNamespaces();
    void ErrorsAreCorrectlyReported() const;
    void ShutdownMemoryLeak() const;
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
    REGISTER_TEST( RemoteRaceWinRemote )
    REGISTER_TEST( AnonymousNamespaces )
    REGISTER_TEST( ErrorsAreCorrectlyReported )
    #if defined( __WINDOWS__ )
        // TODO:LINUX TODO:OSX - Fix and enable this test
        REGISTER_TEST( ShutdownMemoryLeak )
    #endif
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
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = allowRace;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = TEST_PROTOCOL_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( numRemoteWorkers );
    s.Listen( TEST_PROTOCOL_PORT );

    // clean up anything left over from previous runs
    Array< AString > files;
    FileIO::GetFiles( AStackString<>( "../tmp/Test/Distributed" ), AStackString<>( "*.*" ), true, &files );
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
    const char * target( "../tmp/Test/Distributed/dist.lib" );
    TestHelper( target, 1 );
}

// TestWith4RemoteWorkerThreads
//------------------------------------------------------------------------------
void TestDistributed::TestWith4RemoteWorkerThreads() const
{
    const char * target( "../tmp/Test/Distributed/dist.lib" );
    TestHelper( target, 4 );
}

// WithPCH
//------------------------------------------------------------------------------
void TestDistributed::WithPCH() const
{
    const char * target( "../tmp/Test/Distributed/distpch.lib" );
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
        const char * target( "../tmp/Test/Distributed/dist.lib" );
        TestHelper( target, 1, false, true ); // allow race
    }
    {
        const char * target( "../tmp/Test/Distributed/dist.lib" );
        TestHelper( target, 4, false, true ); // allow race
    }
    {
        const char * target( "../tmp/Test/Distributed/distpch.lib" );
        TestHelper( target, 4, false, true ); // allow race
    }
    {
        const char * target( "badcode" );
        TestHelper( target, 4, true, true ); // compilation should fail, allow race
    }
}

// RemoteRaceWinRemote
//------------------------------------------------------------------------------
void TestDistributed::RemoteRaceWinRemote()
{
    // Check that a remote race that is won remotely is correctly handled
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/RemoteRaceWinRemote/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_NoLocalConsumptionOfRemoteJobs = true;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_PORT );

    TEST_ASSERT( fBuild.Build( AStackString<>( "RemoteRaceWinRemote" ) ) );
}

// AnonymousNamespaces
//------------------------------------------------------------------------------
void TestDistributed::AnonymousNamespaces()
{
    // Check that compiling multiple objects with identically named symbols
    // in anonymouse namespaces don't cause link errors.  This is because
    // the MS compiler uses the path to the cpp file being compiled to
    // generate the symbol name (it doesn't respect the #line directives)
    const char * target( "../tmp/Test/Distributed/AnonymousNamespaces/AnonymousNamespaces.lib" );
    TestHelper( target, 1 );
}

// TestForceInclude
//------------------------------------------------------------------------------
void TestDistributed::TestForceInclude() const
{
    const char * target( "../tmp/Test/Distributed/ForceInclude/ForceInclude.lib" );
    TestHelper( target, 4 );
}

// ErrorsAreCorrectlyReported
//------------------------------------------------------------------------------
void TestDistributed::ErrorsAreCorrectlyReported() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/ErrorsAreCorrectlyReported/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = TEST_PROTOCOL_PORT;

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( TEST_PROTOCOL_PORT );

    // MSVC
    #if defined( __WINDOWS__ )
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            // Check that build fails
            TEST_ASSERT( false == fBuild.Build( AStackString<>( "ErrorsAreCorrectlyReported-MSVC" ) ) );

            // Check that error is returned
            TEST_ASSERT( GetRecordedOutput().Find( "error C2143" ) && GetRecordedOutput().Find( "missing ';' before '}'" ) );
        }
    #endif

    // Clang
    #if defined( __WINDOWS__ ) // TODO:B Enable for OSX and Linux
        {
            FBuild fBuild( options );
            TEST_ASSERT( fBuild.Initialize() );

            // Check that build fails
            TEST_ASSERT( false == fBuild.Build( AStackString<>( "ErrorsAreCorrectlyReported-Clang" ) ) );

            // Check that error is returned
            TEST_ASSERT( GetRecordedOutput().Find( "fatal error: expected ';' at end of declaration" ) );
        }
    #endif
}

// ShutdownMemoryLeak
//------------------------------------------------------------------------------
void TestDistributed::ShutdownMemoryLeak() const
{
    // Ensure clean shutdown (no leaks) if the build is aborted and there are
    // available distributable jobs
    //
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/ShutdownMemoryLeak/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = TEST_PROTOCOL_PORT;

    // Init
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // NOTE: No remote server created so jobs stay in m_DistributableJobs_Available queue

    // Create thread that will abort build to simulate Crtl+C or other external stop
    class Helper
    {
    public:
        static uint32_t AbortBuild( void * )
        {
            // Wait until some distribtued jobs are available
            Timer t;
            while ( Job::GetTotalLocalDataMemoryUsage() == 0 )
            {
                Thread::Sleep( 1 );
                TEST_ASSERT( t.GetElapsed() < 5.0f ); // Ensure test doesn't hang if broken
            }

            // Abort the build
            FBuild::Get().AbortBuild();
            return 0;
        }
    };
    Thread::ThreadHandle h = Thread::CreateThread( Helper::AbortBuild );

    // Start build and check it was aborted
    TEST_ASSERT( fBuild.Build( AStackString<>( "ShutdownMemoryLeak" ) ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "FBuild: Error: BUILD FAILED: ShutdownMemoryLeak" ) )

    Thread::WaitForThread( h );
    Thread::CloseHandle( h );
}

// TestZiDebugFormat
//------------------------------------------------------------------------------
void TestDistributed::TestZiDebugFormat() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = TEST_PROTOCOL_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( TEST_PROTOCOL_PORT );

    TEST_ASSERT( fBuild.Build( AStackString<>( "remoteZi" ) ) );
}

// TestZiDebugFormat_Local
//------------------------------------------------------------------------------
void TestDistributed::TestZiDebugFormat_Local() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = TEST_PROTOCOL_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( TEST_PROTOCOL_PORT );

    TEST_ASSERT( fBuild.Build( AStackString<>( "remoteZi" ) ) );
}

// D8049_ToolLongDebugRecord
//------------------------------------------------------------------------------
void TestDistributed::D8049_ToolLongDebugRecord() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = TEST_PROTOCOL_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( TEST_PROTOCOL_PORT );

    TEST_ASSERT( fBuild.Build( AStackString<>( "D8049" ) ) );
}

//------------------------------------------------------------------------------
