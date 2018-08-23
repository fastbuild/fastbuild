// Main
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/Graph/WorkerSettingsNode.h"

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
    bool                         m_OverrideCPUAllocation;
    uint32_t                     m_CPUAllocation;
    bool                         m_OverrideWorkMode;
    WorkerSettingsNode::WorkMode m_WorkMode;
    bool                         m_OverrideStartMinimized;
    bool                         m_StartMinimized;

    // Console mode
    bool m_ConsoleMode;

private:
    void ShowUsageError();
};

//------------------------------------------------------------------------------
