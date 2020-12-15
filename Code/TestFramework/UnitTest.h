// UnitTest.h - interface for a unit test
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "UnitTestManager.h"

// Core
#include "Core/Mem/MemTracker.h" // For MEMTRACKER_ENABLED

// UnitTest - Tests derive from this interface
//------------------------------------------------------------------------------
class UnitTest
{
protected:
    explicit        UnitTest() { m_NextTestGroup = nullptr; }
    inline virtual ~UnitTest() = default;

    virtual void RunTests() = 0;
    virtual const char * GetName() const = 0;

    // Run before and after each test
    virtual void PreTest() const {}
    virtual void PostTest( bool /*passed*/ ) const {}

private:
    friend class UnitTestManager;
    UnitTest * m_NextTestGroup;
};

// Create a no-return helper to improve static analysis
#if defined( __WINDOWS__ )
    __declspec(noreturn) void TestNoReturn();
    #define TEST_NO_RETURN TestNoReturn();
#else
    #define TEST_NO_RETURN
#endif

// Test Assertions
//------------------------------------------------------------------------------
#define TEST_ASSERT( expression )                                   \
    do {                                                            \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                                  \
    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunreachable-code" )       \
        if ( !( expression ) )                                      \
        {                                                           \
            if ( UnitTestManager::AssertFailure(  #expression, __FILE__, __LINE__ ) ) \
            {                                                       \
                BREAK_IN_DEBUGGER;                                      \
            }                                                       \
            TEST_NO_RETURN                                          \
        }                                                           \
    } while ( false )                                               \
    PRAGMA_DISABLE_POP_CLANG_WINDOWS                                \
    PRAGMA_DISABLE_POP_MSVC

#define TEST_ASSERTM( expression, ... )                             \
    do {                                                            \
    PRAGMA_DISABLE_PUSH_MSVC(4127)                                  \
    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunreachable-code" )       \
        if ( !( expression ) )                                      \
        {                                                           \
            if ( UnitTestManager::AssertFailureM(  #expression, __FILE__, __LINE__, __VA_ARGS__ ) ) \
            {                                                       \
                BREAK_IN_DEBUGGER;                                  \
            }                                                       \
            TEST_NO_RETURN                                          \
        }                                                           \
    } while ( false )                                               \
    PRAGMA_DISABLE_POP_CLANG_WINDOWS                                \
    PRAGMA_DISABLE_POP_MSVC

// Test Declarations
//------------------------------------------------------------------------------
#define DECLARE_TESTS                                               \
    virtual void RunTests() override;                               \
    virtual const char * GetName() const override;

#define REGISTER_TESTS_BEGIN( testGroupName )                       \
    void testGroupName##Register()                                  \
    {                                                               \
        UnitTestManager::RegisterTestGroup( new testGroupName );    \
    }                                                               \
    const char * testGroupName::GetName() const                     \
    {                                                               \
        return #testGroupName;                                      \
    }                                                               \
    void testGroupName::RunTests()                                  \
    {                                                               \
        UnitTestManager & utm = UnitTestManager::Get();             \
        (void)utm;

#define REGISTER_TEST( testFunction )                               \
        utm.TestBegin( this, #testFunction );                       \
        testFunction();                                             \
        utm.TestEnd();

#define REGISTER_TESTS_END                                          \
    }

#define REGISTER_TESTGROUP( testGroupName )                         \
        extern void testGroupName##Register();                      \
        testGroupName##Register();

// Memory snapshots
//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
    class TestMemorySnapshot
    {
    public:
        TestMemorySnapshot()
            : m_AllocationId( MemTracker::GetCurrentAllocationId() )
            , m_ActiveAllocationCount( MemTracker::GetCurrentAllocationCount() )
        {}
        uint32_t    m_AllocationId;
        uint32_t    m_ActiveAllocationCount;
    };

    // Take a snapshot of the memory state
    #define TEST_MEMORY_SNAPSHOT( snapshot )                            \
        TestMemorySnapshot snapshot

    // Check for expected or unexpected allocations since a snapshot
    #define TEST_EXPECT_ALLOCATION_EVENTS( snapshot, expected )         \
        {                                                               \
            const uint32_t numAllocs = ( MemTracker::GetCurrentAllocationId() - snapshot.m_AllocationId ); \
            TEST_ASSERTM( numAllocs == expected, "%u alloc(s) instead of %u alloc(s)", numAllocs, expected ); \
        }

    // Test for new active allocations since a snapshot
    #define TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( snapshot, expected )    \
        {                                                               \
            const uint32_t numActiveAllocs = ( MemTracker::GetCurrentAllocationCount() - snapshot.m_ActiveAllocationCount ); \
            TEST_ASSERTM( numActiveAllocs == expected, "%u allocs(s) instead of %u alloc(s)", numActiveAllocs, expected ); \
        }

#else
    #define TEST_MEMORY_SNAPSHOT( snapshot ) (void)0
    #define TEST_EXPECT_ALLOCATION_EVENTS( snapshot, expected )
    #define TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( snapshot, expected )
#endif

//------------------------------------------------------------------------------
