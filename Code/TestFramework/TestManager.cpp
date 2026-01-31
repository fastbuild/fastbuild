// TestManager.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestManager.h"
#include "TestGroup.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/Env/Types.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"
#include "Core/Tracing/Tracing.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Static Data
//------------------------------------------------------------------------------
/*static*/ uint32_t TestManager::s_NumTests( 0 );
/*static*/ TestManager::TestInfo TestManager::s_TestInfos[ kMaxTests ];
/*static*/ TestGroup * TestManager::s_FirstTest = nullptr;

// CONSTRUCTOR
//------------------------------------------------------------------------------
TestManager::TestManager()
{
    // don't buffer output so that logging is complete prior to a crash
    VERIFY( setvbuf( stdout, nullptr, _IONBF, 0 ) == 0 );
    VERIFY( setvbuf( stderr, nullptr, _IONBF, 0 ) == 0 );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
TestManager::~TestManager() = default;

//------------------------------------------------------------------------------
/*static*/ void TestManager::FreeRegisteredTests()
{
    // free all registered tests
    TestGroup * testGroup = s_FirstTest;
    while ( testGroup )
    {
        TestGroup * next = testGroup->m_NextTestGroup;
        FDELETE testGroup;
        testGroup = next;
    }
}

//------------------------------------------------------------------------------
/*static*/ bool TestManager::RegisterTestGroup( TestGroup * testGroup )
{
    // Once any  test is registered, set shutdown cleanup function
    static int32_t sAtExitRegistered = atexit( TestManager::FreeRegisteredTests );
    ASSERT( sAtExitRegistered == 0 );
    (void)sAtExitRegistered;

    // first ever test? place as head of list
    if ( s_FirstTest == nullptr )
    {
        s_FirstTest = testGroup;
        return true;
    }

    // link to end of list
    TestGroup * thisGroup = s_FirstTest;
    for ( ;; )
    {
        ASSERT( thisGroup != testGroup );
        if ( thisGroup->m_NextTestGroup == nullptr )
        {
            thisGroup->m_NextTestGroup = testGroup;
            return true;
        }
        thisGroup = thisGroup->m_NextTestGroup;
    }
}

// RunTests
//------------------------------------------------------------------------------
bool TestManager::RunTests( uint32_t runCount )
{
    ParseCommandLineArgs();

    // In discovery mode?
    if ( mListTestsForDiscovery )
    {
        // Print list of tests. This is in gtest compatible format for
        // free integration in Visual Studio
        TestGroup * group = s_FirstTest;
        while ( group )
        {
            OUTPUT( "%s.\n", group->GetName() );
            group->RunTests( true, Array<AString>() ); // Print instead of running
            group = group->m_NextTestGroup;
        }
        return true;
    }

    // Run tests the required amount of times
    runCount = Math::Max( runCount, m_RunCount );
    for ( uint32_t i = 0; i < runCount; ++i )
    {
        if ( runCount > 0 )
        {
            OUTPUT( "------------------------------------------------------------\n" );
            OUTPUT( " Run %u / %u\n", ( i + 1 ), runCount );
            OUTPUT( "------------------------------------------------------------\n" );
        }
        if ( !RunTestsInternal() )
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
bool TestManager::RunTestsInternal()
{
    // Reset results so RunTests can be called multiple times
    s_NumTests = 0;

    // check for compile time filter
    TestGroup * test = s_FirstTest;
    while ( test )
    {
#ifdef PROFILING_ENABLED
        ProfileManager::Start( test->GetName() );
#endif
        {
            test->RunTests( false, m_TestFilters );
        }
#ifdef PROFILING_ENABLED
        ProfileManager::Stop();
        ProfileManager::Synchronize();
#endif
        test = test->m_NextTestGroup;
    }

    uint32_t numPassed = 0;
    float totalTime = 0.0f;
    for ( size_t i = 0; i < s_NumTests; ++i )
    {
        const TestInfo & info = s_TestInfos[ i ];
        if ( info.m_Passed )
        {
            ++numPassed;
        }
        totalTime += info.m_TimeTaken;
    }
    OUTPUT( "------------------------------------------------------------\n" );
    OUTPUT( "Passed: %u / %u (%u failures) in %2.3fs\n", numPassed, s_NumTests, ( s_NumTests - numPassed ), (double)totalTime );
    OUTPUT( "------------------------------------------------------------\n" );

#ifdef PROFILING_ENABLED
    ProfileManager::SynchronizeNoTag();
#endif

    return ( s_NumTests == numPassed );
}

// TestBegin
//------------------------------------------------------------------------------
void TestManager::TestBegin( TestGroup * testGroup, const char * testName )
{
    // record info for this test
    ASSERT( s_NumTests < kMaxTests );
    TestInfo & info = s_TestInfos[ s_NumTests ];
    info.m_TestGroup = testGroup;
    info.m_TestName = testName;
    ++s_NumTests;

    // Note: gtest compatible output for VS Test Explorer integration
    OUTPUT( "[ RUN      ] %s.%s\n", testGroup->GetName(), testName );

    // Flush the output to ensure that name of the test will be logged in case the test will crash the whole binary.
    fflush( stdout );

    // Note allocation state before test is run
#ifdef MEMTRACKER_ENABLED
    m_CurrentTestAllocationId = MemTracker::GetCurrentAllocationId();
#endif

    m_Timer.Restart();

#ifdef PROFILING_ENABLED
    ProfileManager::Start( testName );
#endif

    testGroup->PreTest();
}

// TestEnd
//------------------------------------------------------------------------------
void TestManager::TestEnd()
{
    TestInfo & info = s_TestInfos[ s_NumTests - 1 ];

    info.m_TestGroup->PostTest();

#ifdef MEMTRACKER_ENABLED
    // Get allocation state here (before profiling, which can cause allocations)
    const uint32_t postAllocationId = MemTracker::GetCurrentAllocationId();
#endif

    // Flush profiling info (track time taken as part of test)
#ifdef PROFILING_ENABLED
    ProfileManager::Stop();
    ProfileManager::Synchronize();
#endif

    const float timeTaken = m_Timer.GetElapsed();

    info.m_TimeTaken = timeTaken;

#ifdef MEMTRACKER_ENABLED
    const bool hasLeaks = MemTracker::HasAllocationsInRange( m_CurrentTestAllocationId, postAllocationId );
    if ( hasLeaks && TestGroup::IsMemoryLeakCheckEnabled() )
    {
        info.m_MemoryLeaks = true;
        OUTPUT( " - Test '%s' in %2.3fs : *** FAILED (Memory Leaks)***\n", info.m_TestName, (double)timeTaken );
        // Note: gtest compatible output for VS Test Explorer integration
        OUTPUT( "[   FAILED ] %s.%s\n", info.m_TestGroup->GetName(), info.m_TestName );
        MemTracker::DumpAllocations( m_CurrentTestAllocationId, postAllocationId );
        if ( IsDebuggerAttached() )
        {
            TEST_ASSERT( false && "Memory leaks detected" );
        }
        return;
    }

    // Disabling leak checks is done per-test so we
    // re-enable it here (each test must re-disable it)
    TestGroup::SetMemoryLeakCheckEnabled( true );
#endif

    // Note: gtest compatible output for VS Test Explorer integration
    OUTPUT( "[       OK ] %s.%s (%ums)\n",
            info.m_TestGroup->GetName(),
            info.m_TestName,
            static_cast<uint32_t>( timeTaken * 1000.0f ) );
    info.m_Passed = true;
}

// AssertFailure
//------------------------------------------------------------------------------
/*static*/ bool TestManager::AssertFailure( const char * message,
                                            const char * file,
                                            uint32_t line )
{
    OUTPUT( "\n-------- TEST ASSERTION FAILED --------\n" );
    OUTPUT( "%s(%u): Assert: %s", file, line, message );
    OUTPUT( "\n-----^^^ TEST ASSERTION FAILED ^^^-----\n" );

    return true; // tell the calling code to break at the failure site
}

// AssertFailureM
//------------------------------------------------------------------------------
/*static*/ bool TestManager::AssertFailureM( const char * message,
                                             const char * file,
                                             uint32_t line,
                                             MSVC_SAL_PRINTF const char * formatString,
                                             ... )
{
    AStackString<4096> buffer;
    va_list args;
    va_start( args, formatString );
    buffer.VFormat( formatString, args );
    va_end( args );

    OUTPUT( "\n-------- TEST ASSERTION FAILED --------\n" );
    OUTPUT( "%s(%u): Assert: %s", file, line, message );
    OUTPUT( "\n%s", buffer.Get() );
    OUTPUT( "\n-----^^^ TEST ASSERTION FAILED ^^^-----\n" );

    return true; // tell the calling code to break at the failure site
}

//------------------------------------------------------------------------------
void TestManager::ParseCommandLineArgs()
{
    // Test can be controlled via command line args:
    //   - listed (for discovery)
    //   - filtered (to run subsets of tests)
    // We emulate behavior of gtest executables so that we are compatible with
    // the gtest adaptor for the Visual Studio Test Explorer.

    // Obtain command line args
    AStackString cmdLine;
    Env::GetCmdLine( cmdLine );
    StackArray<AString> args;
    cmdLine.Tokenize( args, ' ' );

    for ( const AString & arg : args )
    {
        // Discovery arg
        if ( arg.EqualsI( "-List" ) ||
             arg.EqualsI( "--gtest_list_tests" ) ) // gtest style
        {
            // Set flag so RunTests() will print the list of tests instead
            mListTestsForDiscovery = true;
            continue;
        }

        // Filter arg
        if ( arg.BeginsWithI( "-Filter" ) ||
             arg.BeginsWithI( "--gtest_filter=" ) ) // gtest style
        {
            // Extract name of TestGroup so RunTests() only runs that
            // For all tests in a TestGroup, format is:
            //  - ':' delimited
            //  - 'TestGroup.TestName' for a single test
            //  - 'TestGroup.*' for all tests in a group
            // These can be strung together in various combinations
            //
            // NOTE: gtest supports other more complex filtering, but we only
            // implement what's needed to integrate in Visual Studio Test Explorer
            AStackString( arg.Find( '=' ) + 1 ).Tokenize( m_TestFilters, ':' );
            continue;
        }

        // Embed some stings that must be present in the exectuable for the
        // VisualStudio gtest adapater to consider running our tests
        // consider this a gtest executable
        if ( arg == "MagicStringsForGTestCompatibility" )
        {
            OUTPUT( "This program contains tests written using Google Test. You can use the" );
            OUTPUT( "For more information, please read the Google Test documentation at" );
            OUTPUT( "Run only the tests whose name matches one of the positive patterns but" );
        }

        // Run count arg
        uint32_t runCount = 0;
        if ( arg.Scan( "-RunCount=%u", &runCount ) )
        {
            m_RunCount = runCount;
        }
    }
}

//------------------------------------------------------------------------------
