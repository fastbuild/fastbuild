// TestAString.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include <Core/Process/Thread.h>
#include <Core/Time/Timer.h>

// TestTimer
//------------------------------------------------------------------------------
class TestTimer : public UnitTest
{
private:
    DECLARE_TESTS

    void Validate() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestTimer )
    REGISTER_TEST( Validate )
REGISTER_TESTS_END

// Validate
//------------------------------------------------------------------------------
void TestTimer::Validate() const
{
    Timer t;
    t.Start();
    const int64_t before = t.GetNow();
    Thread::Sleep( 10 ); // sleep for 100ms
    const float elapsed = t.GetElapsed();
    const float elapsedMS = t.GetElapsedMS();
    const int64_t after = t.GetNow();

    // some time must have elapsed
    TEST_ASSERT( after > before );

    // sanity check
    TEST_ASSERT( elapsed >= 0.001f ); // at least 1ms
    TEST_ASSERT( elapsed <  1.000f ); // some sensible value

    // sanity check
    TEST_ASSERT( elapsedMS >=    1.0f ); // at least 1ms
    TEST_ASSERT( elapsedMS <  1000.0f ); // some sensible value
}

//------------------------------------------------------------------------------
