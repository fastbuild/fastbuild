// WorkerSettingsNode.h - Worker configuration settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

#include "Core/Containers/Tags.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// WorkerSettingsNode
//------------------------------------------------------------------------------
class WorkerSettingsNode : public Node
{
    REFLECT_NODE_DECLARE( WorkerSettingsNode )
public:
    #define WORKER_SETTINGS_NODE_NAME "WorkerSettings"
    
    explicit WorkerSettingsNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function ) override;
    virtual ~WorkerSettingsNode();

    static inline Node::Type GetTypeS() { return Node::WORKER_SETTINGS_NODE; }

    virtual bool IsAFile() const override;

    // Access to settings
    enum WorkMode : uint8_t
    {
        DISABLED        = 0, // Don't work for anyone
        WHEN_IDLE       = 1, // Work when others when idle
        DEDICATED       = 2  // Work for others always
    };
    WorkMode GetWorkMode() const { return static_cast<WorkMode>( m_WorkModeEnum ); }
    void SetWorkMode( const WorkMode m );

    // CPU Usage limits
    inline uint32_t GetNumCPUsToUse() const { return m_NumCPUsToUse; }
    void SetNumCPUsToUse( const uint32_t c );

    // Start minimized
    inline bool GetStartMinimized() { return m_StartMinimized; }
    void SetStartMinimized( const bool startMinimized );

    const Tags & GetWorkerTags() const;
    void         ApplyWorkerTags( const Tags & workerTags );

    inline bool            GetSandboxEnabled() { return m_SandboxEnabled; }
    void                   SetSandboxEnabled( const bool sandboxEnabled );
    inline const AString & GetSandboxExe() const { return m_SandboxExe; }
    void                   SetSandboxExe( const AString & path );
    const AString &        GetAbsSandboxExe() const;
    inline const AString & GetSandboxArgs() const { return m_SandboxArgs; }
    void                   SetSandboxArgs( const AString & args );
    inline const AString & GetSandboxTmp() const { return m_SandboxTmp; }
    void                   SetSandboxTmp( const AString & path );
    const AString &        GetObfuscatedSandboxTmp() const;
    
private:
    // Exposed settings
    //friend class FunctionWorkerSettings;
    uint32_t         m_FileVersion;
    AString          m_WorkModeString;
    mutable uint8_t  m_WorkModeEnum;
    uint32_t         m_NumCPUsToUse;
    bool             m_StartMinimized;
    Array< AString > m_BaseWorkerTagStrings;
    bool             m_SandboxEnabled;
    AString          m_SandboxExe;
    AString          m_SandboxArgs;
    AString          m_SandboxTmp;

    mutable AString  m_AbsSandboxExe;
    mutable AString  m_ObfuscatedSandboxTmp;
    mutable Tags     m_BaseWorkerTags;
    mutable Tag      m_SandboxTag;
    mutable Tags     m_WorkerTags;
};

//------------------------------------------------------------------------------
