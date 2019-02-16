// Main
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"

// Core
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

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
    bool m_OverrideCPUAllocation;
    uint32_t m_CPUAllocation;
    bool m_OverrideWorkMode;
    WorkerSettings::Mode m_WorkMode;

    // Console mode
    bool m_ConsoleMode;

    AString m_BrokeragePath;

private:
    void ShowUsageError();
};

//------------------------------------------------------------------------------
