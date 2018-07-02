// TestDistributed.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildTest/Tests/FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"  // for NATIVE_SLASH
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
    void WorkerTags_Remote();
    void WorkerTags_Local();
    void AnonymousNamespaces();
    void ErrorsAreCorrectlyReported() const;
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

                m_ServerOptions.m_NumThreadsInJobQueue = 1;
            }
            inline virtual ~HelperOptions() = default;

            AString m_DelPrevFilesWildcard;
            AString m_KeepPrevFilesWildcard;
            bool m_TargetIsAFile = true;
            bool m_CompilationShouldFail = false;
            FBuildTestOptions m_ClientOptions;
            Server::Options m_ServerOptions;
    };

    void TestHelper( const char * target, const HelperOptions & helperOptions ) const;
    void _WorkerTags( const bool remote, const bool clean );
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
    REGISTER_TEST( WorkerTags_Remote )
    REGISTER_TEST( WorkerTags_Local )
    REGISTER_TEST( AnonymousNamespaces )
    REGISTER_TEST( ErrorsAreCorrectlyReported )
    #if defined( __WINDOWS__ )
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
    Server s( helperOptions.m_ServerOptions );
    s.Listen( TEST_PROTOCOL_PORT );

    // clean up anything left over from previous runs
    Array< AString > files;
    FileIO::GetFiles(
        AStackString<>( "../tmp/Test/Distributed" ),
        helperOptions.m_DelPrevFilesWildcard,
        true,  // recurse
        false,  // includeDirs
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

    bool pass = fBuild.Build( AStackString<>( target ) );
    if ( !helperOptions.m_CompilationShouldFail )
    {
        TEST_ASSERT( pass );
    }

    // make sure output file is as expected
    if ( helperOptions.m_TargetIsAFile &&
        !helperOptions.m_CompilationShouldFail )
    {
        TEST_ASSERT( FileIO::FileExists( target ) );
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
    helperOptions.m_ServerOptions.m_NumThreadsInJobQueue = 4;
    TestHelper( target, helperOptions );
}

// WithPCH
//------------------------------------------------------------------------------
void TestDistributed::WithPCH() const
{
    const char * target( "../tmp/Test/Distributed/distpch.lib" );
    HelperOptions helperOptions;
    helperOptions.m_ServerOptions.m_NumThreadsInJobQueue = 4;
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
    helperOptions.m_ServerOptions.m_NumThreadsInJobQueue = 4;
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

    helperOptions.m_ServerOptions.m_NumThreadsInJobQueue = 4;

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


// WorkerTags_Remote
//------------------------------------------------------------------------------
void TestDistributed::WorkerTags_Remote()
{
    const bool remote = true;

    // clean
    bool clean = true;
    _WorkerTags(
        remote,
        clean );

    // dirty
    clean = false;
    _WorkerTags(
        remote,
        clean );
}

// WorkerTags_Local
//------------------------------------------------------------------------------
void TestDistributed::WorkerTags_Local()
{
    const bool remote = false;

    // clean
    bool clean = true;
    _WorkerTags(
        remote,
        clean );

    // dirty
    clean = false;
    _WorkerTags(
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
    helperOptions.m_ServerOptions.m_NumThreadsInJobQueue = 4;
    TestHelper( target, helperOptions );
}

// ErrorsAreCorrectlyReported
//------------------------------------------------------------------------------
void TestDistributed::ErrorsAreCorrectlyReported() const
{
    HelperOptions helperOptions;
    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_CompilationShouldFail = true;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/ErrorsAreCorrectlyReported/fbuild.bff";
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;

    // MSVC
    #if defined( __WINDOWS__ )
        {
            const char * target( "ErrorsAreCorrectlyReported-MSVC" );
            TestHelper( target, helperOptions );

            // Check that error is returned
            TEST_ASSERT( GetRecordedOutput().Find( "error C2143" ) && GetRecordedOutput().Find( "missing ';' before '}'" ) );
        }
    #endif

    // Clang
    #if defined( __WINDOWS__ ) // TODO:B Enable for OSX and Linux
        {
            const char * target( "ErrorsAreCorrectlyReported-Clang" );
            TestHelper( target, helperOptions );

            // Check that error is returned
            TEST_ASSERT( GetRecordedOutput().Find( "fatal error: expected ';' at end of declaration" ) );
        }
    #endif
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
    helperOptions.m_ClientOptions.m_ForceCleanBuild = true;
    TestHelper( target, helperOptions );
}

// _WorkerTags
//------------------------------------------------------------------------------
void TestDistributed::_WorkerTags( const bool remote, const bool clean )
{
    HelperOptions helperOptions;
    helperOptions.m_ClientOptions.m_NoLocalConsumptionOfRemoteJobs = remote;
    helperOptions.m_ClientOptions.m_ForceCleanBuild = clean;

    const char * testTagsTarget( "Test-Tags" );
    const char * dummySandboxExeTarget( "DummySandboxExe" );
    const char * testTagsSandboxTarget( "Test-Tags-Sandbox" );
    const char * testTagsNotSandboxTarget( "Test-Tags-NotSandbox" );

    helperOptions.m_TargetIsAFile = false;
    helperOptions.m_ClientOptions.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestDistributed/WorkerTags/fbuild.bff";

    helperOptions.m_CompilationShouldFail = true;

    // build tags target, with no tags and no sandbox
    {
        TestHelper( testTagsTarget, helperOptions );
    }

    // build tags & sandbox target, with no tags and no sandbox
    {
        TestHelper( testTagsSandboxTarget, helperOptions );
    }

    // build tags & not sandbox target, with no tags and no sandbox
    {
        TestHelper( testTagsNotSandboxTarget, helperOptions );
    }

    Tags workerTags;
    Array< AString > tagStrings;
    tagStrings.Append( AStackString<>( "InstalledCompilerA" ) );
    tagStrings.Append( AStackString<>( "CompileHarness=CH1" ) );
    tagStrings.Append( AStackString<>( "InstalledTesterA" ) );
    tagStrings.Append( AStackString<>( "TestHarness=TH1" ) );
#if defined( __WINDOWS__ )
    tagStrings.Append( AStackString<>( "OS=Win-7-64" ) );  // only used to match against Win-*-* in .bff file
#elif defined ( __APPLE__ )
    tagStrings.Append( AStackString<>( "OS=Mac-OSX-64" ) );  // only used to match against Mac-*-* in .bff file
#elif defined ( __LINUX__ )
    tagStrings.Append( AStackString<>( "OS=Linux-OpenSUSE42-64" ) );  // only used to match against Linux-*-* in .bff file
#endif
    workerTags.ParseAndAddTags( tagStrings );
    workerTags.SetValid( true );
    helperOptions.m_ClientOptions.m_OverrideLocalWorkerTags = true;
    helperOptions.m_ClientOptions.m_LocalWorkerTags = workerTags;

    helperOptions.m_ServerOptions.m_WorkerTags = workerTags;

    // build tags & sandbox target, with tags but no sandbox
    {
        TestHelper( testTagsSandboxTarget, helperOptions );
    }

    helperOptions.m_CompilationShouldFail = false;

    // build tags & not sandbox target, with tags but no sandbox
    {
        TestHelper( testTagsNotSandboxTarget, helperOptions );
    }

    // build tags target, with tags but no sandbox
    {
        TestHelper( testTagsTarget, helperOptions );
    }

    // build dummysandbox exe target, for use below
    {
        TestHelper( dummySandboxExeTarget, helperOptions );
    }

    AStackString<> sandboxExeDir( "../tmp/Test/Distributed/WorkerTags/Sandbox" );
    
    helperOptions.m_ClientOptions.m_OverrideSandboxEnabled = true;
    helperOptions.m_ClientOptions.m_SandboxEnabled = true;
    helperOptions.m_ClientOptions.m_OverrideSandboxExe = true;
    helperOptions.m_ClientOptions.m_SandboxExe = sandboxExeDir;
#if defined( __WINDOWS__ )
    helperOptions.m_ClientOptions.m_SandboxExe += "/dummysandbox.exe";
#elif defined ( __APPLE__ )
    helperOptions.m_ClientOptions.m_SandboxExe += "/dummysandbox";
#elif defined ( __LINUX__ )
    helperOptions.m_ClientOptions.m_SandboxExe += "/dummysandbox";
#endif
    helperOptions.m_ClientOptions.m_OverrideSandboxTmp = true;

    AStackString<> sandboxTmpDir( sandboxExeDir );
    sandboxTmpDir += "/sandboxtmp";

    helperOptions.m_ClientOptions.m_SandboxTmp = sandboxTmpDir;
    helperOptions.m_ClientOptions.m_SandboxTmp += "/client";

    helperOptions.m_ServerOptions.m_SandboxEnabled = true;
    helperOptions.m_ServerOptions.m_SandboxTmp = sandboxTmpDir;
    helperOptions.m_ServerOptions.m_SandboxTmp += "/server";

    // keep the dummysandbox files around for the following sandbox test steps
    helperOptions.m_KeepPrevFilesWildcard = "dummysandbox.*";

    // build tags target, with tags and sandbox
    {
        TestHelper( testTagsTarget, helperOptions );
    }

    // build tags & sandbox target, with tags and sandbox
    {
        TestHelper( testTagsSandboxTarget, helperOptions );
    }

    helperOptions.m_CompilationShouldFail = true;

    // build tags & not sandbox target, with tags and sandbox
    {
        TestHelper( testTagsNotSandboxTarget, helperOptions );
    }
}

//------------------------------------------------------------------------------
