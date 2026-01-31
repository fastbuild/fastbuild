// TestManager
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Env/Assert.h"
#include "Core/Env/MSVCStaticAnalysis.h"
#include "Core/Env/Types.h"
#include "Core/Mem/MemTracker.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class TestGroup;

// TestManager
//------------------------------------------------------------------------------
class TestManager : public Singleton<TestManager>
{
public:
    TestManager();
    ~TestManager();

    // Run tests:
    //   - all tests (default), or
    //   - filtered subset (if -Filter=filter set on cmdline)
    // Run all tests
    //   - once by default, or
    //   - repeatedly (if runCount > 1, or with -RunCount=x on cmdline)
    bool RunTests( uint32_t runCount = 1 );

    // tests register (using the test declaration macros) via this interface
    [[nodiscard]] static bool RegisterTestGroup( TestGroup * testGroup );

    // When tests are being executed, they are wrapped with these
    void TestBegin( TestGroup * testGroup, const char * testName );
    void TestEnd();

    // TEST_ASSERT uses this interface to notify of assertion failures
    static bool AssertFailure( const char * message, const char * file, uint32_t line );
    // TEST_ASSERTM uses this interface to notify of assertion failures
    static bool AssertFailureM( const char * message,
                                const char * file,
                                uint32_t line,
                                MSVC_SAL_PRINTF const char * formatString,
                                ... ) FORMAT_STRING( 4, 5 );

private:
    static void FreeRegisteredTests();
    [[nodiscard]] bool RunTestsInternal();
    void ParseCommandLineArgs();

    Timer m_Timer;

    // Behavior control options
    bool mListTestsForDiscovery = false; // List tests instead of running them
    Array<AString> m_TestFilters; // Run only specified TestGroups of Tests
    uint32_t m_RunCount = 1;

    // Track allocations for tests to catch leaks
#ifdef MEMTRACKER_ENABLED
    uint32_t m_CurrentTestAllocationId = 0;
#endif

    inline static const uint32_t kMaxTests = 1024;
    class TestInfo
    {
    public:
        TestGroup * m_TestGroup = nullptr;
        const char * m_TestName = nullptr;
        bool m_Passed = false;
        bool m_MemoryLeaks = false;
        float m_TimeTaken = 0.0f;
    };
    static uint32_t s_NumTests;
    static TestInfo s_TestInfos[ kMaxTests ];

    static TestGroup * s_FirstTest;
};

//------------------------------------------------------------------------------
