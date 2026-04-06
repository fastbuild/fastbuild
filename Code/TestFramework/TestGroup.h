// TestGroup.h - interface for a group of related tests
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "TestManager.h"

// Core
#include "Core/Mem/MemTracker.h" // For MEMTRACKER_ENABLED
#include "Core/Tracing/Tracing.h"

class TestGroupTest;

//------------------------------------------------------------------------------
class TestGroup
{
protected:
    explicit TestGroup() { m_NextTestGroup = nullptr; }
    virtual ~TestGroup() = default;

    virtual const char * GetName() const = 0;

    // Memory Leak checks can be disabled for individual tests
    static void SetMemoryLeakCheckEnabled( bool enabled ) { sMemoryLeakCheckEnabled = enabled; }
    static bool IsMemoryLeakCheckEnabled() { return sMemoryLeakCheckEnabled; }

private:
    // Test filtering
    [[nodiscard]] bool ShouldRun( const char * test, const Array<AString> & filters ) const;
    void RunTests( bool listOnly, const Array<AString> & filters );

    friend class TestManager;
    friend class TestGroupTest;
    TestGroup * m_NextTestGroup;
    TestGroupTest * m_TestsHead = nullptr;

    static bool sMemoryLeakCheckEnabled;
};

//------------------------------------------------------------------------------
class TestGroupTest
{
public:
    TestGroupTest( TestGroup * testGroup );
    virtual ~TestGroupTest() {}

protected:
    virtual const char * GetName() const = 0;
    virtual void Run() const = 0;
    virtual void PreTest() const {}
    virtual void PostTest() const {}

    friend TestGroup;
    TestGroupTest * m_NextTest = nullptr;
};

// Create a no-return helper to improve static analysis
#if defined( __WINDOWS__ )
__declspec( noreturn ) void TestNoReturn();
    #define TEST_NO_RETURN TestNoReturn();
#else
    #define TEST_NO_RETURN
#endif

// Test Assertions
//------------------------------------------------------------------------------
#define TEST_ASSERT( expression )                                   \
    do                                                              \
    {                                                               \
        PRAGMA_DISABLE_PUSH_MSVC( 4127 )                            \
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunreachable-code" )   \
        if ( !( expression ) )                                      \
        {                                                           \
            if ( TestManager::AssertFailure( #expression, __FILE__, __LINE__ ) ) \
            {                                                       \
                BREAK_IN_DEBUGGER;                                  \
            }                                                       \
            TEST_NO_RETURN                                          \
        }                                                           \
    } while ( false )                                               \
    PRAGMA_DISABLE_POP_CLANG_WINDOWS                                \
    PRAGMA_DISABLE_POP_MSVC

#define TEST_ASSERTM( expression, ... )                             \
    do                                                              \
    {                                                               \
        PRAGMA_DISABLE_PUSH_MSVC( 4127 )                            \
        PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wunreachable-code" )   \
        if ( !( expression ) )                                      \
        {                                                           \
            if ( TestManager::AssertFailureM( #expression, __FILE__, __LINE__, __VA_ARGS__ ) ) \
            {                                                       \
                BREAK_IN_DEBUGGER;                                  \
            }                                                       \
            TEST_NO_RETURN                                          \
        }                                                           \
    } while ( false )                                               \
    PRAGMA_DISABLE_POP_CLANG_WINDOWS                                \
    PRAGMA_DISABLE_POP_MSVC

#define TEST_GROUP( testGroup, testGroupTestBaseClass ) \
    class testGroup##_Group : public TestGroup \
    { \
    public: \
        testGroup##_Group() \
        { \
            sTestGroup = this; \
            TestManager::RegisterTestGroup( this ); \
        } \
        ~testGroup##_Group() override { sTestGroup = nullptr; } \
        virtual const char * GetName() const override { return #testGroup; } \
        inline static testGroup##_Group * sTestGroup = nullptr; \
    } \
    gTestRegister_##testGroup##_Instance; \
    class testGroup##_TestCase_Base : public testGroupTestBaseClass \
    { \
    public: \
        testGroup##_TestCase_Base() \
            : testGroupTestBaseClass( testGroup##_Group::sTestGroup ) \
        {} \
    }; \
    class testGroup : public testGroup##_TestCase_Base

#define TEST_CASE( testGroup, test ) \
    class TestRegister_##testGroup##_##test : public testGroup \
    { \
    public: \
        virtual const char * GetName() const override { return #test; } \
        virtual void Run() const override; \
        void operator=( TestRegister_##testGroup##_##test & ) = delete; \
    } gTestRegister_##testGroup##_##test##_Instance; \
    /*virtual*/ void TestRegister_##testGroup##_##test::Run() const

// Memory snapshots
//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
class TestMemorySnapshot
{
public:
    TestMemorySnapshot()
        : m_AllocationId( MemTracker::GetCurrentAllocationId() )
        , m_ActiveAllocationCount( MemTracker::GetCurrentAllocationCount() )
    {
    }

    uint32_t m_AllocationId;
    uint32_t m_ActiveAllocationCount;
};

    // Take a snapshot of the memory state
    #define TEST_MEMORY_SNAPSHOT( snapshot )                            \
        const TestMemorySnapshot snapshot

    // Check for expected or unexpected allocations since a snapshot
    #define TEST_EXPECT_ALLOCATION_EVENTS( snapshot, expected )         \
        {                                                               \
            const uint32_t numAllocs = ( MemTracker::GetCurrentAllocationId() - snapshot.m_AllocationId ); \
            TEST_ASSERTM( numAllocs == expected, "%u alloc(s) instead of %u alloc(s)", numAllocs, expected ); \
        }

    // Test for new active allocations since a snapshot
    #define TEST_EXPECT_INCREASED_ACTIVE_ALLOCATIONS( snapshot, expected ) \
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
