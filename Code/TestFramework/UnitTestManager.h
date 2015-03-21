// UnitTestManager
//------------------------------------------------------------------------------
#pragma once
#ifndef TESTFRAMEWORK_UNITTESTMANAGER_H
#define TESTFRAMEWORK_UNITTESTMANAGER_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class UnitTest;

// UnitTestManager
//------------------------------------------------------------------------------
class UnitTestManager
{
public:
	UnitTestManager();
	~UnitTestManager();

	// run all tests, or tests from a group
	bool RunTests( const char * testGroup = nullptr );

	// singleton behaviour
	#ifdef RELEASE
		static inline UnitTestManager &	Get() { return *s_Instance; }
	#else
		static		  UnitTestManager &	Get();
	#endif
	static inline bool					IsValid() { return ( s_Instance != 0 ); }

	// tests register (using the test declaration macros) via this interface
	static void RegisterTestGroup( UnitTest * testGroup );

	// When tests are being executed, they are wrapped with these
	void TestBegin( const char * testName );
	void TestEnd();

	// TEST_ASSERT uses this interface to notify of assertion failures
	static bool AssertFailure( const char * message, const char * file, uint32_t line );

private:
	uint32_t	m_TestsRun;
	uint32_t	m_TestsPassed;
	const char * m_CurrentTestName;
	Timer		m_Timer;

	static UnitTestManager * s_Instance;
	static UnitTest * s_FirstTest; 
};

//------------------------------------------------------------------------------
#endif // TESTFRAMEWORK_UNITTESTMANAGER_H
