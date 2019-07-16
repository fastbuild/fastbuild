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
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"

// Defines
//------------------------------------------------------------------------------
#define TEST_PROTOCOL_PORT ( Protocol::PROTOCOL_PORT + 1 ) // Avoid conflict with real worker

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
    void TestExe_Remote();
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

    class HelperOptions
    {
        public:
            inline explicit HelperOptions() {
                m_DelPrevFilesWildcard += "*.*";
                m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/fbuild.bff";
                m_ClientOptions.m_AllowDistributed = true;
                m_ClientOptions.m_NumWorkerThreads = 1;
                m_ClientOptions.m_NoLocalConsumptionOfRemoteJobs = true; // ensure jobs happen on the remote worker
                m_ClientOptions.m_AllowLocalRace = false;
                m_ClientOptions.m_EnableMonitor = true; // make sure monitor code paths are tested as well
                m_ClientOptions.m_ForceCleanBuild = false;
                m_ClientOptions.m_DistributionPort = TEST_PROTOCOL_PORT;
            }
            inline virtual ~HelperOptions() = default;

            AString m_DelPrevFilesWildcard;
            AString m_KeepPrevFilesWildcard;
            uint32_t m_ServerNumThreadsInJobQueue = 1;
            bool m_TargetIsAFile = true;
            bool m_CompilationShouldFail = false;
            FBuildTestOptions m_ClientOptions;
    };

    void TestHelper( const char * target, const HelperOptions & helperOptions ) const;
    void _TestExe( const bool remote, const bool clean );
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
    REGISTER_TEST( TestExe_Remote )
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
REGISTER_TESTS_END

// Test
//------------------------------------------------------------------------------
void TestDistributed::TestHelper(
    const char * target, 
    const HelperOptions & helperOptions ) const
{
    FBuild fBuild( helperOptions.m_ClientOptions );

    TEST_ASSERT( fBuild.Initialize() );

    // start a server to emulate the other end
    Server s( helperOptions.m_ServerNumThreadsInJobQueue );
    s.Listen( TEST_PROTOCOL_PORT );

    // clean up anything left over from previous runs
    Array< AString > files;
    FileIO::GetFiles(
        AStackString<>( "../tmp/Test/Distributed" ),
        helperOptions.m_DelPrevFilesWildcard,
        true,  // recurse
        &files );
    const AString * iter = files.Begin();
    const AString * const end = files.End();
    for ( ; iter != end; ++iter )
    {
        bool deleteFile = true;  // first assume true
        if ( !helperOptions.m_KeepPrevFilesWildcard.IsEmpty() )
        {
            AStackString<> filename;
            const char * lastSlash = iter->FindLast( NATIVE_SLASH );
            if ( lastSlash )
            {
                filename += lastSlash + 1;
            }
            else
            {
                filename = *iter;
            }
            if ( filename.Matches( helperOptions.m_KeepPrevFilesWildcard.Get() ) )
            {
                deleteFile = false;
            }
        }
        if ( deleteFile )
        {
            FileIO::FileDelete( iter->Get() );
        }
    }

    if ( helperOptions.m_TargetIsAFile &&
        !helperOptions.m_CompilationShouldFail )
    {
        TEST_ASSERT( FileIO::FileExists( target ) == false );
    }

    bool pass = fBuild.Build( target );
    if ( !helperOptions.m_CompilationShouldFail )
    {
        TEST_ASSERT( pass );

        // make sure all output files are as expected
        if ( helperOptions.m_TargetIsAFile )
        {
            TEST_ASSERT( FileIO::FileExists( target ) );
        }
    }
}

// TestWith1RemoteWorkerThread
//------------------------------------------------------------------------------
void TestDistributed::TestWith1RemoteWorkerThread() const
{
    const char * target( "../tmp/Test/Distributed/dist.lib" );
    HelperOptions helperOptions;
    TestHelper( target, helperOptions );
}

// TestWith4RemoteWorkerThreads
//------------------------------------------------------------------------------
void TestDistributed::TestWith4RemoteWorkerThreads() const
{
    const char * target( "../tmp/Test/Distributed/dist.lib" );
    HelperOptions helperOptions;
    helperOptions.m_ServerNumThreadsInJobQueue = 4;
    TestHelper( target, helperOptions );
}

// WithPCH
//------------------------------------------------------------------------------
void TestDistributed::WithPCH() const
{
    const char * target( "../tmp/Test/Distributed/distpch.lib" );
    HelperOptions helperOptions;
    helperOptions.m_ServerNumThreadsInJobQueue = 4;
    TestHelper( target, helperOptions );
}

// RegressionTest_RemoteCrashOnErrorFormatting
//------------------------------------------------------------------------------
void TestDistributed::RegressionTest_RemoteCrashOnErrorFormatting()
{
    const char * target( "badcode" );
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_CompilationShouldFail = true;
    TestHelper( target, helperOptions );
}

//------------------------------------------------------------------------------
void TestDistributed::TestLocalRace()
{
    HelperOptions helperOptions;
    helperOptions.m_ClientOptions.m_AllowLocalRace = true;

    {
        const char * target( "../tmp/Test/Distributed/dist.lib" );
        TestHelper( target, helperOptions );
    }

    helperOptions.m_ServerNumThreadsInJobQueue = 4;

    {
        const char * target( "../tmp/Test/Distributed/dist.lib" );
        TestHelper( target, helperOptions );
    }
    {
        const char * target( "../tmp/Test/Distributed/distpch.lib" );
        TestHelper( target, helperOptions );
    }
    {
        const char * target( "badcode" );
        helperOptions.m_TargetIsAFile = false;
        helperOptions.m_CompilationShouldFail = true;
        helperOptions.m_ServerNumThreadsInJobQueue = 1;
        TestHelper( target, helperOptions );
    }
}

// RemoteRaceWinRemote
//------------------------------------------------------------------------------
void TestDistributed::RemoteRaceWinRemote()
{
    // Check that a remote race that is won remotely is correctly handled

    const char * target( "RemoteRaceWinRemote" );
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/RemoteRaceWinRemote/fbuild.bff";
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;
    TestHelper( target, helperOptions );
}

// TestExe_Remote
//------------------------------------------------------------------------------
void TestDistributed::TestExe_Remote()
{
    const bool remote = true;

    // clean
    bool clean = true;
    _TestExe(
        remote,
        clean );

    // dirty
    clean = false;
    _TestExe(
        remote,
        clean );
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
    HelperOptions helperOptions;
    TestHelper( target, helperOptions );
}

// TestForceInclude
//------------------------------------------------------------------------------
void TestDistributed::TestForceInclude() const
{
    const char * target( "../tmp/Test/Distributed/ForceInclude/ForceInclude.lib" );
    HelperOptions helperOptions;
    TestHelper( target, helperOptions );
}

// ErrorsAreCorrectlyReported_MSVC
//------------------------------------------------------------------------------
void TestDistributed::ErrorsAreCorrectlyReported_MSVC() const
{
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_CompilationShouldFail = true;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/ErrorsAreCorrectlyReported/fbuild.bff";
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;

    const char * target( "ErrorsAreCorrectlyReported-MSVC" );
    TestHelper( target, helperOptions );

    // Check that error is returned
    TEST_ASSERT( GetRecordedOutput().Find( "error C2143" ) && GetRecordedOutput().Find( "missing ';' before '}'" ) );
}

// ErrorsAreCorrectlyReported_Clang
//------------------------------------------------------------------------------
void TestDistributed::ErrorsAreCorrectlyReported_Clang() const
{
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_CompilationShouldFail = true;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/ErrorsAreCorrectlyReported/fbuild.bff";
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;

    const char * target( "ErrorsAreCorrectlyReported-Clang" );
    TestHelper( target, helperOptions );

    // Check that error is returned
    TEST_ASSERT( GetRecordedOutput().Find( "fatal error: expected ';' at end of declaration" ) );
}

// WarningsAreCorrectlyReported_MSVC
//------------------------------------------------------------------------------
void TestDistributed::WarningsAreCorrectlyReported_MSVC() const
{
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/WarningsAreCorrectlyReported/fbuild.bff";
    helperOptions.m_ClientOptions.m_AllowLocalRace = false;
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;

    const char * target( "WarningsAreCorrectlyReported-MSVC" );
    TestHelper( target, helperOptions );

    // Check that error is returned
    TEST_ASSERT( GetRecordedOutput().Find( "warning C4101" ) && GetRecordedOutput().Find( "'x': unreferenced local variable" ) );
}

// WarningsAreCorrectlyReported_Clang
//------------------------------------------------------------------------------
void TestDistributed::WarningsAreCorrectlyReported_Clang() const
{
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/WarningsAreCorrectlyReported/fbuild.bff";
    helperOptions.m_ClientOptions.m_AllowLocalRace = false;
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;

    const char * target( "WarningsAreCorrectlyReported-Clang" );
    TestHelper( target, helperOptions );

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
    options.m_DistributionPort = TEST_PROTOCOL_PORT;

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
            Timer t;
            float timeout = 5.0f;
            #if __has_feature( thread_sanitizer ) || defined( __SANITIZE_THREAD__ )
                // Code under ThreadSanitizer runs several time slower than normal, so we need a larger timeout.
                timeout = 30.0f;
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
    TEST_ASSERT( GetRecordedOutput().Find( "FBuild: Error: BUILD FAILED: ShutdownMemoryLeak" ) )

    Thread::WaitForThread( h );
    Thread::CloseHandle( h );

    TEST_ASSERT( detectedDistributedJobs );
}

// TestZiDebugFormat
//------------------------------------------------------------------------------
void TestDistributed::TestZiDebugFormat() const
{
    const char * target( "remoteZi" );
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;
    TestHelper( target, helperOptions );
}

// TestZiDebugFormat_Local
//------------------------------------------------------------------------------
void TestDistributed::TestZiDebugFormat_Local() const
{
    const char * target( "remoteZi" );
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_NoLocalConsumptionOfRemoteJobs = false;  // allow local
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;
    TestHelper( target, helperOptions );
}

// D8049_ToolLongDebugRecord
//------------------------------------------------------------------------------
void TestDistributed::D8049_ToolLongDebugRecord() const
{
    const char * target( "D8049" );
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/D8049_ToolLongDebugRecord/fbuild.bff";
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;
    TestHelper( target, helperOptions );
}

// _TestExe
//------------------------------------------------------------------------------
void TestDistributed::_TestExe( const bool remote, const bool clean )
{
    HelperOptions helperOptions;
    helperOptions.m_ClientOptions.m_NoLocalConsumptionOfRemoteJobs = remote;
    helperOptions.m_ClientOptions.m_ForceCleanBuild = clean;

    const char * testTarget( "Test" );

    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/TestExe/fbuild.bff";

    helperOptions.m_CompilationShouldFail = false;

    // build target
    {
        TestHelper( testTarget, helperOptions );
    }
}

//------------------------------------------------------------------------------
