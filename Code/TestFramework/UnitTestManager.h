// UnitTestManager
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Assert.h"
#include "Core/Env/MSVCStaticAnalysis.h"
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
        static inline UnitTestManager & Get() { return *s_Instance; }
    #else
        static        UnitTestManager & Get();
    #endif
    static inline bool IsValid() { return ( s_Instance != nullptr ); }

    // tests register (using the test declaration macros) via this interface
    static void RegisterTestGroup( UnitTest * testGroup );

    // When tests are being executed, they are wrapped with these
    void TestBegin( UnitTest * testGroup, const char * testName );
    void TestEnd();

    // TEST_ASSERT uses this interface to notify of assertion failures
    static bool AssertFailure( const char * message, const char * file, uint32_t line );
    // TEST_ASSERTM uses this interface to notify of assertion failures
    static bool AssertFailureM( const char* message, const char* file, uint32_t line, MSVC_SAL_PRINTF const char* formatString, ... ) FORMAT_STRING( 4, 5 );

private:
    Timer       m_Timer;

    enum : uint32_t { MAX_TESTS = 1024 };
    class TestInfo
    {
    public:
        UnitTest *      m_TestGroup = nullptr;
        const char *    m_TestName = nullptr;
        bool            m_Passed = false;
        bool            m_MemoryLeaks = false;
        float           m_TimeTaken = 0.0f;
    };
    static uint32_t     s_NumTests;
    static TestInfo     s_TestInfos[ MAX_TESTS ];

    static UnitTestManager * s_Instance;
    static UnitTest * s_FirstTest;
};

//------------------------------------------------------------------------------
