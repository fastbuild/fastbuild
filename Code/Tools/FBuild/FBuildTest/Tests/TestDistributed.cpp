// TestDistributed.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"

// Defines
//------------------------------------------------------------------------------
#if !defined( __has_feature )
    #define __has_feature( ... ) 0
#endif

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
    #if defined( DEBUG )
        void RemoteRaceSystemFailure();
    #endif
    void AnonymousNamespaces();
    void ErrorsAreCorrectlyReported_MSVC() const;
    void ErrorsAreCorrectlyReported_Clang() const;
    void WarningsAreCorrectlyReported_MSVC() const;
    void WarningsAreCorrectlyReported_Clang() const;
    void ShutdownMemoryLeak() const;
    void TestForceInclude() const;
    void TestZiDebugFormat() const;
    void TestZiDebugFormat_Local() const;
    void D8049_ToolLongDebugRecord() const;
    void CleanMessageToPreventMSBuildFailure() const;

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
    #if defined( DEBUG )
        REGISTER_TEST( RemoteRaceSystemFailure )
    #endif
    REGISTER_TEST( AnonymousNamespaces )
    REGISTER_TEST( ShutdownMemoryLeak )
    #if defined( __WINDOWS__ )
        REGISTER_TEST( ErrorsAreCorrectlyReported_MSVC ) // TODO:B Enable for OSX and Linux
        REGISTER_TEST( ErrorsAreCorrectlyReported_Clang ) // TODO:B Enable for OSX and Linux
        REGISTER_TEST( WarningsAreCorrectlyReported_MSVC ) // TODO:B Enable for OSX and Linux
        REGISTER_TEST( WarningsAreCorrectlyReported_Clang ) // TODO:B Enable for OSX and Linux
        REGISTER_TEST( TestForceInclude )
        REGISTER_TEST( TestZiDebugFormat )
        REGISTER_TEST( TestZiDebugFormat_Local )
        REGISTER_TEST( D8049_ToolLongDebugRecord )
    #endif
    REGISTER_TEST( CleanMessageToPreventMSBuildFailure )
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
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( numRemoteWorkers );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

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

    const bool pass = fBuild.Build( target );
    if ( !shouldFail )
    {
        TEST_ASSERT( pass );

        // make sure all output files are as expected
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
    TestHelper( target, 1, true ); // compilation should fail
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
        TestHelper( target, 1, true, true ); // compilation should fail, allow race
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
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    TEST_ASSERT( fBuild.Build( "RemoteRaceWinRemote" ) );
}

// RemoteRaceSystemFailure
//------------------------------------------------------------------------------
#if defined( ENABLE_FAKE_SYSTEM_FAILURE )
void TestDistributed::RemoteRaceSystemFailure()
{
    // NOTE: Test only available in DEBUG due to SetFakeSystemFailure

    // Check that a remote race that is won remotely with a failure is
    // correctly handled
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/RemoteRaceSystemFailure/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_NoLocalConsumptionOfRemoteJobs = true;
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;
    options.m_DistVerbose = true;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    // Force a system failure when compiling remotely
    ObjectNode::SetFakeSystemFailureForNextJob();

    // Build
    TEST_ASSERT( fBuild.Build( "RemoteRaceSystemFailure" ) );

    // Ensure the remote failure was induced correctly
    TEST_ASSERT( GetRecordedOutput().Find( "Injecting system failure (sFakeSystemFailure)" ) );
}
#endif

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
    TestHelper( target, 1 );
}

// ErrorsAreCorrectlyReported_MSVC
//------------------------------------------------------------------------------
void TestDistributed::ErrorsAreCorrectlyReported_MSVC() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/ErrorsAreCorrectlyReported/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    // Check that build fails
    TEST_ASSERT( false == fBuild.Build( "ErrorsAreCorrectlyReported-MSVC" ) );

    // Check that error is returned
    TEST_ASSERT( GetRecordedOutput().Find( "error C2143" ) && GetRecordedOutput().Find( "missing ';' before '}'" ) );
}

// ErrorsAreCorrectlyReported_Clang
//------------------------------------------------------------------------------
void TestDistributed::ErrorsAreCorrectlyReported_Clang() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/ErrorsAreCorrectlyReported/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    // Check that build fails
    TEST_ASSERT( false == fBuild.Build( "ErrorsAreCorrectlyReported-Clang" ) );

    // Check that error is returned
    TEST_ASSERT( GetRecordedOutput().Find( "fatal error: expected ';' at end of declaration" ) );
}

// WarningsAreCorrectlyReported_MSVC
//------------------------------------------------------------------------------
void TestDistributed::WarningsAreCorrectlyReported_MSVC() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/WarningsAreCorrectlyReported/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    // Check that build passes
    TEST_ASSERT( fBuild.Build( "WarningsAreCorrectlyReported-MSVC" ) );

    // Check that error is returned
    TEST_ASSERT( GetRecordedOutput().Find( "warning C4101" ) && GetRecordedOutput().Find( "'x': unreferenced local variable" ) );
}

// WarningsAreCorrectlyReported_Clang
//------------------------------------------------------------------------------
void TestDistributed::WarningsAreCorrectlyReported_Clang() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/WarningsAreCorrectlyReported/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;

    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    // Check that build passes
    TEST_ASSERT( fBuild.Build( "WarningsAreCorrectlyReported-Clang" ) );

    // Check that error is returned
    TEST_ASSERT( GetRecordedOutput().Find( "warning: unused variable 'x' [-Wunused-variable]" ) );
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
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;

    // Init
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // NOTE: No remote server created so jobs stay in m_DistributableJobs_Available queue

    // Create thread that will abort build to simulate Crtl+C or other external stop
    class Helper
    {
    public:
        static uint32_t AbortBuild( void * data )
        {
            // Wait until some distributed jobs are available
            const Timer t;
            #if __has_feature( thread_sanitizer ) || defined( __SANITIZE_THREAD__ )
                // Code under ThreadSanitizer runs several time slower than normal, so we need a larger timeout.
                const float timeout = 30.0f;
            #else
                const float timeout = 5.0f;
            #endif
            while ( t.GetElapsed() < timeout )
            {
                if ( Job::GetTotalLocalDataMemoryUsage() != 0 )
                {
                    *static_cast< bool * >( data ) = true;
                    break;
                }
                Thread::Sleep( 1 );
            }

            // Abort the build
            FBuild::Get().AbortBuild();
            return 0;
        }
    };
    bool detectedDistributedJobs = false;
    Thread::ThreadHandle h = Thread::CreateThread( Helper::AbortBuild, nullptr, 64 * KILOBYTE, &detectedDistributedJobs );

    // Start build and check it was aborted
    TEST_ASSERT( fBuild.Build( "ShutdownMemoryLeak" ) == false );
    TEST_ASSERT( GetRecordedOutput().Find( "FBuild: Error: BUILD FAILED: ShutdownMemoryLeak" ) );

    Thread::WaitForThread( h );
    Thread::CloseHandle( h );

    TEST_ASSERT( detectedDistributedJobs );
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
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    TEST_ASSERT( fBuild.Build( "remoteZi" ) );
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
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    TEST_ASSERT( fBuild.Build( "remoteZi" ) );
}

// D8049_ToolLongDebugRecord
//------------------------------------------------------------------------------
void TestDistributed::D8049_ToolLongDebugRecord() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/D8049_ToolLongDebugRecord/fbuild.bff";
    options.m_AllowDistributed = true;
    options.m_NumWorkerThreads = 1;
    options.m_NoLocalConsumptionOfRemoteJobs = true; // ensure all jobs happen on the remote worker
    options.m_AllowLocalRace = false;
    options.m_ForceCleanBuild = true;
    options.m_EnableMonitor = true; // make sure monitor code paths are tested as well
    options.m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;
    FBuild fBuild( options );

    TEST_ASSERT( fBuild.Initialize() );

    // start a client to emulate the other end
    Server s( 1 );
    s.Listen( Protocol::PROTOCOL_TEST_PORT );

    TEST_ASSERT( fBuild.Build( "D8049" ) );
}

//------------------------------------------------------------------------------
void TestDistributed::CleanMessageToPreventMSBuildFailure() const
{
    // Error should be indentical except for a single remove colon
    {
        const AStackString<> in( "C:\\Windows\\TEMP\\.fbuild.tmp\\0x00000000\\"
                                 "core_1018\\F436D72E\\Module.MergeActors.cpp :"
                                 " fatal error C1083: Cannot open compiler intermediate"
                                 " file: 'C:\\Windows\\TEMP\\_CL_7a0408f6in':"
                                 " No such file or directory" );
        AStackString<> expectedOut( in );
        TEST_ASSERT( expectedOut.Replace( "fatal error C1083:", "fatal error C1083 " ) == 1 );
        AStackString<> out;
        Node::CleanMessageToPreventMSBuildFailure( in, out );
        TEST_ASSERT( out == expectedOut );
    }

    // String may contain multiple lines
    {
        const AStackString<> in( "PROBLEM: C:\\Users\\Franta\\AppData\\Local\\Temp\\.fbuild.tmp\\0x96c448b7\\core_1001\\ErrorWithPercent.obj\n"
                                 "ErrorWithPercent.cpp\n"
                                 "C:\\p4\\depot\\Code\\Tools\\FBuild\\FBuildTest\\Data\\TestDistributed\\BadCode\\ErrorWithPercent.cpp(6,11): error C3193: '%': requires '/clr' or '/ZW' command line option\n"
                                 "    int %s%s%s%s\n"
                                 "          ^\n"
                                 "C:\\p4\\depot\\Code\\Tools\\FBuild\\FBuildTest\\Data\\TestDistributed\\BadCode\\ErrorWithPercent.cpp(6,11): error C2143: syntax error: missing ';' before '%'\n"
                                 "    int %s%s%s%s\n"
                                 "          ^\n"
                                 "C:\\p4\\depot\\Code\\Tools\\FBuild\\FBuildTest\\Data\\TestDistributed\\BadCode\\ErrorWithPercent.cpp(6,11): error C2530: 's': references must be initialized\n"
                                 "    int %s%s%s%s\n"
                                 "          ^\n"
                                 "C:\\p4\\depot\\Code\\Tools\\FBuild\\FBuildTest\\Data\\TestDistributed\\BadCode\\ErrorWithPercent.cpp(6,13): error C3071: operator '%' can only be applied to an instance of a ref class or a value-type\n"
                                 "    int %s%s%s%s\n"
                                 "            ^\n"
                                 "C:\\p4\\depot\\Code\\Tools\\FBuild\\FBuildTest\\Data\\TestDistributed\\BadCode\\ErrorWithPercent.cpp(7,1): error C2143: syntax error: missing ';' before '}'\n"
                                 "}\n"
                                 "^\n"
                                 "C:\\p4\\depot\\Code\\Tools\\FBuild\\FBuildTest\\Data\\TestDistributed\\BadCode\\ErrorWithPercent.cpp(6,15): warning C4552: '%': result of expression not used\n"
                                 "    int %s%s%s%s\n"
                                 "              ^" );
        AStackString<> expectedOut( in );
        TEST_ASSERT( expectedOut.Replace( "error C3193:", "error C3193 " ) == 1 );
        TEST_ASSERT( expectedOut.Replace( "error C2143:", "error C2143 " ) == 2 );
        TEST_ASSERT( expectedOut.Replace( "error C2530:", "error C2530 " ) == 1 );
        TEST_ASSERT( expectedOut.Replace( "error C3071:", "error C3071 " ) == 1 );
        TEST_ASSERT( expectedOut.Replace( "warning C4552:", "warning C4552 " ) == 1 );
        AStackString<> out;
        Node::CleanMessageToPreventMSBuildFailure( in, out );
        TEST_ASSERT( out == expectedOut );
    }

    // Make sure it doesn't fail on other input (should leave string alone)
    {
        // empty
        AStackString<> out;
        Node::CleanMessageToPreventMSBuildFailure( AString::GetEmpty(), out );
        TEST_ASSERT( out.IsEmpty() );
    }
    {
        // path
        const AStackString<> in( "C:\\Windows\\TEMP\\_CL_7a0408f6in" );
        AStackString<> out;
        Node::CleanMessageToPreventMSBuildFailure( in, out );
        TEST_ASSERT( out == in );
    }
    {
        // error but not in expected format
        const AStackString<> in( "error C1083 and some other bits" );
        AStackString<> out;
        Node::CleanMessageToPreventMSBuildFailure( in, out );
        TEST_ASSERT( out == in );
    }
    {
        // keyword only
        const AStackString<> in( "error" );
        AStackString<> out;
        Node::CleanMessageToPreventMSBuildFailure( in, out );
        TEST_ASSERT( out == in );
    }
}

//------------------------------------------------------------------------------
