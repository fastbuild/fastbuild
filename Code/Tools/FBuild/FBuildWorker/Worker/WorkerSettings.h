// WorkerSettings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Singleton.h"
#include "Core/Containers/Tags.h"
#include "Core/Strings/AString.h"
#include "Tools/FBuild/FBuildCore/Graph/WorkerSettingsNode.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerSettings
//------------------------------------------------------------------------------
class WorkerSettings : public Singleton< WorkerSettings >
{
public:
    explicit WorkerSettings();
    ~WorkerSettings();

    inline WorkerSettingsNode::WorkMode GetWorkMode() const { return m_WorkerSettingsNode->GetWorkMode(); }
    void SetWorkMode( const WorkerSettingsNode::WorkMode m );

    // CPU Usage limits
    inline uint32_t GetNumCPUsToUse() const { return m_WorkerSettingsNode->GetNumCPUsToUse(); }
    void SetNumCPUsToUse( const uint32_t c );

    // Start minimzed
    inline bool GetStartMinimized() { return m_WorkerSettingsNode->GetStartMinimized(); }
    void SetStartMinimized( const bool startMinimized );

    inline bool GetSandboxEnabled() { return m_WorkerSettingsNode->GetSandboxEnabled(); }
    void SetSandboxEnabled( const bool sandboxEnabled );
    inline const AString & GetSandboxExe() const { return m_WorkerSettingsNode->GetSandboxExe(); }
    void SetSandboxExe( const AString & path );
    inline const AString & GetAbsSandboxExe() const { return m_WorkerSettingsNode->GetAbsSandboxExe(); }
    inline const AString & GetSandboxArgs() const { return m_WorkerSettingsNode->GetSandboxArgs(); }
    void SetSandboxArgs( const AString & args );
    inline const AString & GetSandboxTmp() const { return m_WorkerSettingsNode->GetSandboxTmp(); }
    inline const AString & GetObfuscatedSandboxTmp() const
        { return m_WorkerSettingsNode->GetObfuscatedSandboxTmp(); }
    void SetSandboxTmp( const AString & path );
    
    inline const Tags & GetWorkerTags() const { return m_WorkerSettingsNode->GetWorkerTags(); }
    void ApplyWorkerTags( const Tags & workerTags );

    void Load();
    void Save();
private:
    NodeGraph * m_NodeGraph;
    WorkerSettingsNode * m_WorkerSettingsNode;
    bool m_WorkerSettingsNodeOwned;
};

//------------------------------------------------------------------------------
