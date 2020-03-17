// TestMutex.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Process/SystemMutex.h"
#include "Core/Strings/AStackString.h"

#if !defined( __has_feature )
    #define __has_feature( ... ) 0
#endif

// TestMutex
//------------------------------------------------------------------------------
class TestSystemMutex : public UnitTest
{
private:
    DECLARE_TESTS

    void LeakRegression() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestSystemMutex )
    REGISTER_TEST( LeakRegression )
REGISTER_TESTS_END

// LeakRegression
//------------------------------------------------------------------------------
void TestSystemMutex::LeakRegression() const
{
    const uint32_t testCount( 1024 * 20 );

    // Create a unique name based on architecture/config to avoid
    // collisions when tests can run in parallel
    AStackString<> mutexName( "SysMutexName_" );
    #if defined( __clang__ )
        mutexName += "Clang";
    #endif
    mutexName += (sizeof(void*) == 8) ? "64_" : "32_";
    #if __has_feature( address_sanitizer ) || defined( __SANITIZE_ADDRESS__ )
        mutexName += "ASan";
    #elif __has_feature( memory_sanitizer )
        mutexName += "MSan";
    #elif defined( DEBUG )
        mutexName += "Debug";
    #elif defined( RELEASE )
        #if defined( PROFILING_ENABLED )
            mutexName += "Profile";
        #else
            mutexName += "Release";
        #endif
    #endif

    SystemMutex m( mutexName.Get() );
    for ( uint32_t i = 0; i < testCount; ++i )
    {
        TEST_ASSERT( m.TryLock() );
        m.Unlock();
    }
}

//------------------------------------------------------------------------------
