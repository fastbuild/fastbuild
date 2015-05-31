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
#include "Core/Strings/AString.h"
#include "Core/Tracing/Tracing.h"

#include <string.h>
#if defined( __WINDOWS__ )
    #include <windows.h>
#endif

// Static Data
//------------------------------------------------------------------------------
/*static*/ UnitTestManager * UnitTestManager::s_Instance = nullptr;
/*static*/ UnitTest * UnitTestManager::s_FirstTest = nullptr;

// CONSTRUCTOR
//------------------------------------------------------------------------------
UnitTestManager::UnitTestManager()
: m_TestsRun( 0 )
, m_TestsPassed( 0 )
, m_CurrentTestName( nullptr )
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
			AssertHandler::SetThrowOnAssert( true );
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
			AssertHandler::SetThrowOnAssert( false );
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
loop:
	if ( thisGroup->m_NextTestGroup == nullptr )
	{
		thisGroup->m_NextTestGroup = testGroup;
		return;
	}
	thisGroup = thisGroup->m_NextTestGroup;
	goto loop;
}

// RunTests
//------------------------------------------------------------------------------
bool UnitTestManager::RunTests( const char * testGroup )
{
	m_TestsRun = 0;
	m_TestsPassed = 0;

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
		try
		{
			test->RunTests();
		}
		catch (...)
		{
			OUTPUT( " - Test '%s' *** FAILED ***\n", m_CurrentTestName );
		}
		test = test->m_NextTestGroup;
	}

	OUTPUT( "------------------------------\n" );
	OUTPUT( "Summary For All Tests\n" );
	OUTPUT( " - Passed: %u\n", m_TestsPassed );
	OUTPUT( " - Failed: %u\n", (m_TestsRun - m_TestsPassed ) );
	OUTPUT( "------------------------------\n" );

	return ( m_TestsRun == m_TestsPassed );
}

// TestBegin
//------------------------------------------------------------------------------
void UnitTestManager::TestBegin( const char * testName )
{
	m_TestsRun++;
	OUTPUT( " - Test '%s'\n", testName );
	m_CurrentTestName = testName;
	#ifdef MEMTRACKER_ENABLED
		MemTracker::Reset();
	#endif
	m_Timer.Start();
}

// TestEnd
//------------------------------------------------------------------------------
void UnitTestManager::TestEnd()
{
    #ifdef PROFILING_ENABLED
        // flush the profiling buffers, but don't profile the sync itself
        // (because it leaves an outstanding memory alloc, it looks like a leak)
        ProfileManager::SynchronizeNoTag();
    #endif

	float timeTaken = m_Timer.GetElapsed();

	#ifdef MEMTRACKER_ENABLED
		if ( MemTracker::GetCurrentAllocationCount() != 0 )
		{
			OUTPUT( " - Test '%s' in %2.3fs : *** FAILED (Memory Leaks)***\n", m_CurrentTestName, timeTaken );
			MemTracker::DumpAllocations();
			TEST_ASSERT( false );
			return;
		}
	#endif

	OUTPUT( " - Test '%s' in %2.3fs : PASSED\n", m_CurrentTestName, timeTaken );
	m_TestsPassed++;
}

// Assert
//------------------------------------------------------------------------------
/*static*/ bool UnitTestManager::AssertFailure( const char * message,
											    const char * file,
												uint32_t line )
{
	OUTPUT( "\n-------- TEST ASSERTION FAILED --------\n" );
	OUTPUT( "%s(%i): Assert: %s", file, line, message );
	OUTPUT( "\n-----^^^ TEST ASSERTION FAILED ^^^-----\n" );

	if ( IsDebuggerAttached() )
	{
		return true; // tell the calling code to break at the failure sight
	}

	// throw will be caught by the unit test framework and noted as a failure
	throw "Test Failed";
}

//------------------------------------------------------------------------------
