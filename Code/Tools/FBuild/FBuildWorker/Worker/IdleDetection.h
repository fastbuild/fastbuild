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
    void Update( uint32_t idleThresholdPercent );

    // query status
    inline bool IsIdle() const { return m_IsIdle; }
    inline float IsIdleFloat() const { return m_IsIdleFloat; }

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

    bool IsIdleInternal( uint32_t idleThresholdPercent, float & idleCurrent );

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
    float   m_IsIdleFloat;
    float   m_IsIdleCurrent;
    int32_t m_IdleSmoother;
    int32_t m_IdleFloatSmoother;
    Array< ProcessInfo > m_ProcessesInOurHierarchy;
    uint64_t m_LastTimeIdle;
    uint64_t m_LastTimeBusy;
};

//------------------------------------------------------------------------------
