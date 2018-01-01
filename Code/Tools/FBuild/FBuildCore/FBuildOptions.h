// FBuild.cpp - The main FBuild interface class
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"

// Core
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

// FBuildOptions
//------------------------------------------------------------------------------
struct FBuildOptions
{
public:
    FBuildOptions();

    enum OptionsResult
    {
        OPTIONS_OK,
        OPTIONS_OK_AND_QUIT,
        OPTIONS_ERROR
    };
    OptionsResult ProcessCommandLine( int argc, char * argv[] );

    enum WrapperMode
    {
        WRAPPER_MODE_NONE,
        WRAPPER_MODE_MAIN_PROCESS,
        WRAPPER_MODE_INTERMEDIATE_PROCESS,
        WRAPPER_MODE_FINAL_PROCESS
    };

    void SetWorkingDir( const AString & path );
    inline const AString & GetWorkingDir() const { return m_WorkingDir; }

    const AString& GetArgs() const { return m_Args; }

    // Basic Args
    AString     m_ProgramName;
    AString     m_Args; // Stored copy of args
    WrapperMode m_WrapperMode                       = WRAPPER_MODE_NONE;

    // Targets
    Array< AString > m_Targets;

    // Build Behaviour
    bool        m_ForceCleanBuild                   = false;
    bool        m_StopOnFirstError                  = true;
    bool        m_FastCancel                        = false;
    bool        m_WaitMode                          = false;
    bool        m_DisplayTargetList                 = false;
    bool        m_DisplayDependencyDB               = false;

    // Cache
    bool        m_UseCacheRead                      = false;
    bool        m_UseCacheWrite                     = false;
    bool        m_CacheInfo                         = false;
    uint32_t    m_CacheTrim                         = 0;

    // Distributed Compilation
    bool        m_AllowDistributed                  = false;
    bool        m_DistVerbose                       = false;
    bool        m_NoLocalConsumptionOfRemoteJobs    = false;
    bool        m_AllowLocalRace                    = true;
    uint16_t    m_DistributionPort                  = Protocol::PROTOCOL_PORT;

    // General Output
    bool        m_ShowInfo                          = false;
    bool        m_ShowCommandLines                  = false;
    bool        m_ShowBuildCommands                 = true;
    bool        m_ShowErrors                        = true;
    bool        m_ShowProgress                      = false;
    bool        m_ShowSummary                       = false;
    bool        m_NoSummaryOnError                  = false;
    bool        m_GenerateReport                    = false;
    bool        m_EnableMonitor                     = false;

    // DB saving
    bool        m_SaveDBOnCompletion                = false;
    bool        m_FixupErrorPaths                   = false;

    uint32_t    m_NumWorkerThreads                  = 0; // True default detected in constructor
    AString     m_ConfigFile;

    inline uint32_t GetWorkingDirHash() const                   { return m_WorkingDirHash; }
    inline const AString & GetMainProcessMutexName() const      { return m_ProcessMutexName; }
    inline const AString & GetFinalProcessMutexName( ) const    { return m_FinalProcessMutexName; }
    inline const AString & GetSharedMemoryName() const          { return m_SharedMemoryName; }

private:
    void DisplayHelp( const AString & programName ) const;
    void DisplayVersion() const;

    AString     m_WorkingDir;
    uint32_t    m_WorkingDirHash                    = 0;
    AString     m_ProcessMutexName;
    AString     m_FinalProcessMutexName;
    AString     m_SharedMemoryName;
};

//------------------------------------------------------------------------------
