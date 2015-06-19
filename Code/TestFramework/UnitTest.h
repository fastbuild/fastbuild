// UnitTest.h - interface for a unit test
//------------------------------------------------------------------------------
#pragma once
#ifndef TESTFRAMEWORK_UNITTEST_H
#define TESTFRAMEWORK_UNITTEST_H

#include "UnitTestManager.h"

// UnitTest - Tests derive from this interface
//------------------------------------------------------------------------------
class UnitTest
{
protected:
	explicit		UnitTest() { m_NextTestGroup = nullptr; }
	inline virtual ~UnitTest() {}

	virtual void RunTests() = 0;
	virtual const char * GetName() const = 0;

private:
	friend class UnitTestManager;
	UnitTest * m_NextTestGroup;
};

// macros
//------------------------------------------------------------------------------
#define TEST_ASSERT( expression )									\
	do {															\
	PRAGMA_DISABLE_PUSH_MSVC(4127)									\
		if ( !( expression ) )										\
		{															\
			if ( UnitTestManager::AssertFailure(  #expression, __FILE__, __LINE__ ) ) \
			{														\
				BREAK_IN_DEBUGGER;										\
			}														\
		}															\
	} while ( false );												\
	PRAGMA_DISABLE_POP_MSVC

#define DECLARE_TESTS												\
	virtual void RunTests();										\
	virtual const char * GetName() const;

#define REGISTER_TESTS_BEGIN( testGroupName )						\
	void testGroupName##Register()									\
	{																\
		UnitTestManager::RegisterTestGroup( new testGroupName );	\
	}																\
	const char * testGroupName::GetName() const						\
	{																\
		return #testGroupName;										\
	}																\
	void testGroupName::RunTests()									\
	{																\
		UnitTestManager & utm = UnitTestManager::Get();				\
		(void)utm;

#define REGISTER_TEST( testFunction )								\
		utm.TestBegin( this, #testFunction );					    \
		testFunction();												\
		utm.TestEnd();

#define REGISTER_TESTS_END											\
	}

#define REGISTER_TESTGROUP( testGroupName )							\
		extern void testGroupName##Register();						\
		testGroupName##Register();

//------------------------------------------------------------------------------
#endif // TESTFRAMEWORK_UNITTEST_H
