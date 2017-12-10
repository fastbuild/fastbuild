// IdleDetection
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Env/Types.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerWindow
//------------------------------------------------------------------------------
class IdleDetection : public Singleton< IdleDetection >
{
public:
    explicit IdleDetection();
    ~IdleDetection();

    // returns true if idle
    void Update();

    // query status
    inline bool IsIdle() const { return m_IsIdle; }

private:
    // struct to track processes with
    struct ProcessInfo
    {
        inline bool operator == ( uint32_t pid ) const { return m_PID == pid; }

        uint32_t    m_PID;
        uint32_t    m_AliveValue;
        #if defined( __WINDOWS__ )
            void *      m_ProcessHandle;
        #endif
        uint64_t    m_LastTime;
    };

    bool IsIdleInternal();

    static void GetSystemTotalCPUUsage( uint64_t & outIdleTime,
                                        uint64_t & outKernTime,
                                        uint64_t & outUserTime );
    static void GetProcessTime( const ProcessInfo & pi,
                                uint64_t & outKernTime,
                                uint64_t & outUserTime );
    void UpdateProcessList();
    #if defined( __LINUX__ )
        static bool GetProcessInfoString( const char * fileName,
                                          AStackString< 1024 > & outProcessInfoString );
    #endif

    Timer   m_Timer;
    float   m_CPUUsageFASTBuild;
    float   m_CPUUsageTotal;
    bool    m_IsIdle;
    int32_t m_IdleSmoother;
    Array< ProcessInfo > m_ProcessesInOurHierarchy;
    uint64_t m_LastTimeIdle;
    uint64_t m_LastTimeBusy;
};

//------------------------------------------------------------------------------
