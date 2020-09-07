// UnitTestManager.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "UnitTestManager.h"
#include "UnitTest.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Mem/MemTracker.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"
#include "Core/Tracing/Tracing.h"

#include <stdio.h>
#include <string.h>
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Static Data
//------------------------------------------------------------------------------
/*static*/ uint32_t UnitTestManager::s_NumTests( 0 );
/*static*/ UnitTestManager::TestInfo UnitTestManager::s_TestInfos[ MAX_TESTS ];
/*static*/ UnitTestManager * UnitTestManager::s_Instance = nullptr;
/*static*/ UnitTest * UnitTestManager::s_FirstTest = nullptr;

// OnAssert callback
//------------------------------------------------------------------------------
/*static*/
#if defined( __WINDOWS__ )
    __declspec(noreturn)
#endif
void OnAssert( const char * /*message*/ )
{
    throw "Assert Failed";
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
UnitTestManager::UnitTestManager()
{
    // manage singleton
    ASSERT( s_Instance == nullptr );
    s_Instance = this;

    // if we're running outside the debugger, we don't want
    // failures to pop up a dialog.  We want them to throw so
    // the test framework can catch the exception
    #ifdef ASSERTS_ENABLED
        if ( IsDebuggerAttached() == false )
        {
            AssertHandler::SetAssertCallback( OnAssert );
        }
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
UnitTestManager::~UnitTestManager()
{
    #ifdef ASSERTS_ENABLED
        if ( IsDebuggerAttached() == false )
        {
            AssertHandler::SetAssertCallback( nullptr );
        }
    #endif

    // free all registered tests
    UnitTest * testGroup = s_FirstTest;
    while ( testGroup )
    {
        UnitTest * next = testGroup->m_NextTestGroup;
        FDELETE testGroup;
        testGroup = next;
    }

    // manage singleton
    ASSERT( s_Instance == this );
    s_Instance = nullptr;
}

// Get
//------------------------------------------------------------------------------
#ifdef DEBUG
    /*static*/ UnitTestManager & UnitTestManager::Get()
    {
        ASSERT( s_Instance );
        return *s_Instance;
    }
#endif

// RegisterTest
//------------------------------------------------------------------------------
/*static*/ void UnitTestManager::RegisterTestGroup( UnitTest * testGroup )
{
    // first ever test? place as head of list
    if ( s_FirstTest == nullptr )
    {
        s_FirstTest = testGroup;
        return;
    }

    // link to end of list
    UnitTest * thisGroup = s_FirstTest;
    for ( ;; )
    {
        if ( thisGroup->m_NextTestGroup == nullptr )
        {
            thisGroup->m_NextTestGroup = testGroup;
            return;
        }
        thisGroup = thisGroup->m_NextTestGroup;
    }
}

// RunTests
//------------------------------------------------------------------------------
bool UnitTestManager::RunTests( const char * testGroup )
{
    // Reset results so RunTests can be called multiple times
    s_NumTests = 0;

    // check for compile time filter
    UnitTest * test = s_FirstTest;
    while ( test )
    {
        if ( testGroup != nullptr )
        {
            // is this test the one we want?
            if ( AString::StrNCmp( test->GetName(), testGroup, strlen( testGroup ) ) != 0 )
            {
                // no -skip it
                test = test->m_NextTestGroup;
                continue;
            }
        }

        OUTPUT( "------------------------------\n" );
        OUTPUT( "Test Group: %s\n", test->GetName() );
        #ifdef PROFILING_ENABLED
            ProfileManager::Start( test->GetName() );
        #endif
        try
        {
            test->RunTests();
        }
        catch ( ... )
        {
            OUTPUT( " - Test '%s' *** FAILED ***\n", s_TestInfos[ s_NumTests - 1 ].m_TestName );
            s_TestInfos[ s_NumTests - 1 ].m_TestGroup->PostTest( false );
        }
        #ifdef PROFILING_ENABLED
            ProfileManager::Stop();
            ProfileManager::Synchronize();
        #endif
        test = test->m_NextTestGroup;
    }

    OUTPUT( "------------------------------------------------------------\n" );
    OUTPUT( "Summary For All Tests\n" );
    uint32_t numPassed = 0;
    float totalTime = 0.0f;
    UnitTest * lastGroup = nullptr;
    for ( size_t i = 0; i < s_NumTests; ++i )
    {
        const TestInfo & info = s_TestInfos[ i ];
        if ( info.m_TestGroup != lastGroup )
        {
            OUTPUT( "------------------------------------------------------------\n" );
            OUTPUT( "             : %s\n", info.m_TestGroup->GetName() );
            lastGroup = info.m_TestGroup;
        }

        const char * status = "OK";
        if ( info.m_Passed )
        {
            ++numPassed;
        }
        else
        {
            status = ( info.m_MemoryLeaks ) ? "FAIL (LEAKS)" : "FAIL";
        }

        OUTPUT( "%12s : %5.3fs : %s\n", status, (double)info.m_TimeTaken, info.m_TestName );
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
void UnitTestManager::TestBegin( UnitTest * testGroup, const char * testName )
{
    // record info for this test
    ASSERT( s_NumTests < MAX_TESTS );
    TestInfo & info = s_TestInfos[ s_NumTests ];
    info.m_TestGroup = testGroup;
    info.m_TestName = testName;
    ++s_NumTests;

    OUTPUT( " - Test '%s'\n", testName );
    // Flush the output to ensure that name of the test will be logged in case the test will crash the whole binary.
    fflush( stdout );

    #ifdef MEMTRACKER_ENABLED
        MemTracker::Reset();
    #endif
    m_Timer.Start();

    #ifdef PROFILING_ENABLED
        ProfileManager::Start( testName );
    #endif

    testGroup->PreTest();
}

// TestEnd
//------------------------------------------------------------------------------
void UnitTestManager::TestEnd()
{
    TestInfo & info = s_TestInfos[ s_NumTests - 1 ];

    info.m_TestGroup->PostTest( true );

    #ifdef MEMTRACKER_ENABLED
        // Get allocation count here (before profiling, which can cause allocations)
        const uint32_t posAllocationCount = MemTracker::GetCurrentAllocationCount();
    #endif

    // Flush profiling info (track time taken as part of test)
    #ifdef PROFILING_ENABLED
        ProfileManager::Stop();
        ProfileManager::Synchronize();
    #endif

    float timeTaken = m_Timer.GetElapsed();

    info.m_TimeTaken = timeTaken;

    #ifdef MEMTRACKER_ENABLED
        if ( posAllocationCount != 0 )
        {
            info.m_MemoryLeaks = true;
            OUTPUT( " - Test '%s' in %2.3fs : *** FAILED (Memory Leaks)***\n", info.m_TestName, (double)timeTaken );
            MemTracker::DumpAllocations();
            if ( IsDebuggerAttached() )
            {
                TEST_ASSERT( false && "Memory leaks detected" );
            }
            return;
        }
    #endif

    OUTPUT( " - Test '%s' in %2.3fs : PASSED\n", info.m_TestName, (double)timeTaken );
    info.m_Passed = true;
}

// AssertFailure
//------------------------------------------------------------------------------
/*static*/ bool UnitTestManager::AssertFailure( const char * message,
                                                const char * file,
                                                uint32_t line )
{
    OUTPUT( "\n-------- TEST ASSERTION FAILED --------\n" );
    OUTPUT( "%s(%u): Assert: %s", file, line, message );
    OUTPUT( "\n-----^^^ TEST ASSERTION FAILED ^^^-----\n" );

    if ( IsDebuggerAttached() )
    {
        return true; // tell the calling code to break at the failure sight
    }

    // throw will be caught by the unit test framework and noted as a failure
    throw "Test Failed";
}

// AssertFailureM
//------------------------------------------------------------------------------
/*static*/ bool UnitTestManager::AssertFailureM( const char * message,
                                                 const char * file,
                                                 uint32_t line,
                                                 MSVC_SAL_PRINTF const char * formatString,
                                                 ... )
{
    AStackString< 4096 > buffer;
    va_list args;
    va_start( args, formatString );
    buffer.VFormat( formatString, args );
    va_end( args );

    OUTPUT( "\n-------- TEST ASSERTION FAILED --------\n" );
    OUTPUT( "%s(%u): Assert: %s", file, line, message );
    OUTPUT( "\n%s", buffer.Get() );
    OUTPUT( "\n-----^^^ TEST ASSERTION FAILED ^^^-----\n" );

    if ( IsDebuggerAttached() )
    {
        return true; // tell the calling code to break at the failure sight
    }

    // throw will be caught by the unit test framework and noted as a failure
    throw "Test Failed";
}

//------------------------------------------------------------------------------
