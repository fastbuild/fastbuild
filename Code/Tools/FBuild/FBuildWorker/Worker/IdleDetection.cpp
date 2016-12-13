// Worker
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "IdleDetection.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Math/Conversions.h"
//#include "Core/Tracing/Tracing.h"

// system
#if defined( __WINDOWS__ )
    #include <windows.h>
    #include <tlhelp32.h>
#endif

// Defines
//------------------------------------------------------------------------------
#define IDLE_DETECTION_THRESHOLD_PERCENT ( 10.0f )
#define IDLE_CHECK_DELAY_SECONDS ( 0.1f )

// CONSTRUCTOR
//------------------------------------------------------------------------------
IdleDetection::IdleDetection() :
    #if defined( __WINDOWS__ )
        m_CPUUsageFASTBuild( 0.0f ),
        m_CPUUsageTotal( 0.0f ),
    #endif
      m_IsIdle( false )
    , m_IdleSmoother( 0 )
    , m_ProcessesInOurHierarchy( 32, true )
    #if defined( __WINDOWS__ )
        , m_LastTimeIdle( 0 )
        , m_LastTimeBusy( 0 )
    #endif
{
    #if defined( __WINDOWS__ )
        ProcessInfo self;
        self.m_PID = ::GetCurrentProcessId();
        self.m_AliveValue = 0;
        self.m_ProcessHandle = ::GetCurrentProcess();
        self.m_LastTime = 0;
        m_ProcessesInOurHierarchy.Append( self );
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
IdleDetection::~IdleDetection() = default;

// Update
//------------------------------------------------------------------------------
void IdleDetection::Update()
{
    // apply smoothing based on current "idle" state
    if ( IsIdleInternal() )
    {
        ++m_IdleSmoother;
    }
    else
    {
        m_IdleSmoother -= 2; // become non-idle more quickly than we become idle
    }
    m_IdleSmoother = Math::Clamp(m_IdleSmoother, 0, 50);

    // change state only when at extreme of either end of scale
    if (m_IdleSmoother == 50)
    {
        m_IsIdle = true;
    }
    if (m_IdleSmoother == 0)
    {
        m_IsIdle = false;
    }
}

//
//------------------------------------------------------------------------------
bool IdleDetection::IsIdleInternal()
{
    #if defined( __APPLE__ )
        ASSERT( false ); // TODO:MAC Implement IdleDetection::IsIdleInternal
        return false;
    #elif defined( __LINUX__ )
        ASSERT( false ); // TODO:LINUX Implement IdleDetection::IsIdleInternal
        return false;
    #else
        // determine total cpu time (including idle)
        uint64_t systemTime = 0;
        {
            FILETIME ftIdle, ftKern, ftUser;
            VERIFY(::GetSystemTimes(&ftIdle, &ftKern, &ftUser));
            const uint64_t idleTime = ((uint64_t)ftIdle.dwHighDateTime << 32) | (uint64_t)ftIdle.dwLowDateTime;
            const uint64_t kernTime = ((uint64_t)ftKern.dwHighDateTime << 32) | (uint64_t)ftKern.dwLowDateTime;
            const uint64_t userTime = ((uint64_t)ftUser.dwHighDateTime << 32) | (uint64_t)ftUser.dwLowDateTime;
            if ( m_LastTimeBusy > 0 )
            {
                uint64_t idleTimeDelta = (idleTime - m_LastTimeIdle);
                uint64_t usedTimeDelta = ((userTime + kernTime - idleTime) - m_LastTimeBusy);
                systemTime = (idleTimeDelta + usedTimeDelta);
                m_CPUUsageTotal = (float)((double)usedTimeDelta / (double)systemTime) * 100.0f;
            }
            m_LastTimeIdle = (idleTime);
            m_LastTimeBusy = (userTime + kernTime - idleTime);
        }

        // if the total CPU time is below the idle theshold, we don't need to
        // check to know acurately what the cpu use of FASTBuild is
        if ( m_CPUUsageTotal < IDLE_DETECTION_THRESHOLD_PERCENT )
        {
            return true;
        }

        // reduce check frequency
        if (m_Timer.GetElapsed() > IDLE_CHECK_DELAY_SECONDS )
        {
            // iterate all processes
            HANDLE hSnapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnapShot != INVALID_HANDLE_VALUE)
            {
                static uint32_t aliveValue(0);
                aliveValue++;

                PROCESSENTRY32 thProcessInfo;
                memset(&thProcessInfo, 0, sizeof(PROCESSENTRY32));
                thProcessInfo.dwSize = sizeof(PROCESSENTRY32);
                while (Process32Next(hSnapShot, &thProcessInfo) != FALSE)
                {
                    uint32_t parentPID = thProcessInfo.th32ParentProcessID;

                    // is process a child of one we care about?
                    if (m_ProcessesInOurHierarchy.Find(parentPID))
                    {
                        const uint32_t pid = thProcessInfo.th32ProcessID;
                        ProcessInfo * info = m_ProcessesInOurHierarchy.Find(pid);
                        if (info)
                        {
                            // an existing process that is still alive
                            info->m_AliveValue = aliveValue; // still active
                        }
                        else
                        {
                            // a new process
                            void * handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
                            if (handle)
                            {
                                // track new process
                                ProcessInfo newProcess;
                                newProcess.m_PID = pid;
                                newProcess.m_ProcessHandle = handle;
                                newProcess.m_AliveValue = aliveValue;
                                newProcess.m_LastTime = 0;
                                m_ProcessesInOurHierarchy.Append(newProcess);
                            }
                            else
                            {
                                // gracefully handle failure to open proces
                                // maybe it closed before we got to it
                            }
                        }
                    }
                }
                CloseHandle(hSnapShot);

                // prune dead processes
                {
                    // never prune first process (this process)
                    const size_t numProcesses = m_ProcessesInOurHierarchy.GetSize();
                    for (size_t i = (numProcesses - 1); i > 0; --i)
                    {
                        if (m_ProcessesInOurHierarchy[i].m_AliveValue != aliveValue)
                        {
                            // dead process
                            m_ProcessesInOurHierarchy.EraseIndex(i);
                        }
                    }
                }

                // accumulate cpu usage for processes we care about
                if (systemTime) // skip first update
                {
                    float totalPerc(0.0f);

                    const auto end = m_ProcessesInOurHierarchy.End();
                    auto it = m_ProcessesInOurHierarchy.Begin();
                    while (it != end)
                    {
                        FILETIME ftProcKern, ftProcUser, ftUnused;
                        ProcessInfo & pi = (*it);
                        if (::GetProcessTimes(pi.m_ProcessHandle,
                            &ftUnused,      // creation time
                            &ftUnused,      // exit time
                            &ftProcKern,    // kernel time
                            &ftProcUser))   // user time
                        {
                            const uint64_t kernTime = ((uint64_t)ftProcKern.dwHighDateTime << 32) | (uint64_t)ftProcKern.dwLowDateTime;
                            const uint64_t userTime = ((uint64_t)ftProcUser.dwHighDateTime << 32) | (uint64_t)ftProcUser.dwLowDateTime;
                            const uint64_t totalTime = (userTime + kernTime);
                            const uint64_t lastTime = pi.m_LastTime;
                            if (lastTime != 0) // ignore first update
                            {
                                const uint64_t timeSpent = (totalTime - lastTime);
                                float perc = (float)((double)timeSpent / (double)systemTime) * 100.0f;
                                totalPerc += perc;
                            }
                            pi.m_LastTime = totalTime;
                        }
                        else
                        {
                            // gracefully handle - process may have exited
                        }
                        ++it;
                    }

                    m_CPUUsageFASTBuild = totalPerc;
                }
            }

            m_Timer.Start();
        }

        return ( ( m_CPUUsageTotal - m_CPUUsageFASTBuild ) < IDLE_DETECTION_THRESHOLD_PERCENT );
    #endif
}

//------------------------------------------------------------------------------
