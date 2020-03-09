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
        bool m_IsSubprocess;    // Process is child? (Internal)
        bool m_UseSubprocess;   // Should we launch a sub-process?
    #endif

    // resource usage
    bool                 m_OverrideCPUAllocation;
    uint32_t             m_CPUAllocation;
    bool                 m_OverrideWorkMode;
    WorkerSettings::Mode m_WorkMode;
    uint32_t             m_MinimumFreeMemoryMiB; // Minimum OS free memory including virtual memory to let worker do its work
    bool                 m_OverrideStartMinimized;
    bool                 m_StartMinimized;
    bool                 m_OverrideSandboxEnabled;
    bool                 m_SandboxEnabled;
    bool                 m_OverrideSandboxExe;
    AString              m_SandboxExe;
    bool                 m_OverrideSandboxArgs;
    AString              m_SandboxArgs;
    bool                 m_OverrideSandboxTmp;
    AString              m_SandboxTmp;

    // Console mode
    bool m_ConsoleMode;

private:
    void ShowUsageError();
};

//------------------------------------------------------------------------------
