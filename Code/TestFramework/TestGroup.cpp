// TestGroup.cpp - interface for a group of related tests
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestGroup.h"

// Core
#include <Core/Strings/AStackString.h>
#include <Core/Tracing/Tracing.h>

// StaticData
//------------------------------------------------------------------------------
/*static*/ bool TestGroupTest::sMemoryLeakCheckEnabled = false;

//------------------------------------------------------------------------------
void TestGroup::RunTests( bool listOnly, const Array<AString> & filters )
{
    TestManager & utm = TestManager::Get();

    TestGroupTest * test = m_TestsHead;
    while ( test )
    {
        const char * const testName = test->GetName();
        if ( listOnly )
        {
            OUTPUT( "  %s\n", testName ); // Note gtest indent format
        }
        else if ( ShouldRun( testName, filters ) )
        {
            utm.TestBegin( this, testName );
            test->PreTest();
            test->Run();
            test->PostTest();
            utm.TestEnd();
        }
        test = test->m_NextTest;
    }
}

//------------------------------------------------------------------------------
bool TestGroup::ShouldRun( const char * test, const Array<AString> & filters ) const
{
    // If there are no filters, all tests are run
    if ( filters.IsEmpty() )
    {
        return true;
    }

    // Pattern match filters against qualified test names
    AStackString qualifiedTestName;
    qualifiedTestName.Format( "%s.%s", GetName(), test );
    for ( const AString & filter : filters )
    {
        if ( qualifiedTestName.MatchesI( filter.Get() ) )
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
TestGroupTest::TestGroupTest( TestGroup * testGroup )
{
    // Handle empty list
    if ( testGroup->m_TestsHead == nullptr )
    {
        // Special case for empty list
        testGroup->m_TestsHead = this;
        return;
    }

    // Append to end of list
    TestGroupTest * test = testGroup->m_TestsHead;
    for ( ;; )
    {
        if ( test->m_NextTest == nullptr )
        {
            test->m_NextTest = this;
            return;
        }
        test = test->m_NextTest;
    }
}

// TestNoReturn
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
void TestNoReturn()
{
    #if defined( __clang__ )
    for ( ;; )
    {
    }
    #endif
}
#endif

//------------------------------------------------------------------------------
