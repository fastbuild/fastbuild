// Main
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"

// Core
#include "Core/Env/Types.h"

// Forward Declaration
//------------------------------------------------------------------------------
class AString;

// FBuildWorkerOptions
//------------------------------------------------------------------------------
class FBuildWorkerOptions
{
public:
    FBuildWorkerOptions();

    bool ProcessCommandLine( const AString & commandLine );

    // sub-process control
#if defined( __WINDOWS__ )
    bool m_IsSubprocess = false; // Process is child? (Internal)
    bool m_UseSubprocess = true; // Should we launch a sub-process?
#endif

    // resource usage
    bool m_OverrideCPUAllocation = false;
    uint32_t m_CPUAllocation = 0;
    bool m_OverrideWorkMode = false;
    WorkerSettings::Mode m_WorkMode = WorkerSettings::WHEN_IDLE;
    uint32_t m_MinimumFreeMemoryMiB = 0; // Minimum OS free memory including virtual memory to let worker do its work

    // Console mode
    bool m_ConsoleMode = false;

    // Other
    bool m_PeriodicRestart = false;

private:
    void ShowUsageError();
};

//------------------------------------------------------------------------------
