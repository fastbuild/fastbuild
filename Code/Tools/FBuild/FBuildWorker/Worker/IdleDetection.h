// IdleDetection
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Env/Types.h"
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
    bool IsIdleInternal();

    Timer   m_Timer;
    #if defined( __WINDOWS__ )
        float   m_CPUUsageFASTBuild;
        float   m_CPUUsageTotal;
    #endif
    bool    m_IsIdle;
    int32_t m_IdleSmoother;

    // struct to track processes with
    struct ProcessInfo
    {
        inline bool operator == ( uint32_t pid ) const { return m_PID == pid; }

        uint32_t    m_PID;
        uint32_t    m_AliveValue;
        void *      m_ProcessHandle;
        uint64_t    m_LastTime;
    };
    Array< ProcessInfo > m_ProcessesInOurHierarchy;

    #if defined( __WINDOWS__ )
        uint64_t m_LastTimeIdle;
        uint64_t m_LastTimeBusy;
    #endif
};

//------------------------------------------------------------------------------
