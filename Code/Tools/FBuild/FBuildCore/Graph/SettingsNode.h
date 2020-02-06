// SettingsNode.h - Global configuration settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Function;

// SettingsNode
//------------------------------------------------------------------------------
class SettingsNode : public Node
{
    REFLECT_NODE_DECLARE( SettingsNode )
public:
    explicit SettingsNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~SettingsNode() override;

    static inline Node::Type GetTypeS() { return Node::SETTINGS_NODE; }

    virtual bool IsAFile() const override;

    // Access to settings
    const AString &                 GetCachePath() const;
    const AString &                 GetCachePathMountPoint() const;
    const AString &                 GetCachePluginDLL() const;
    inline const Array< AString > & GetWorkerList() const { return m_Workers; }
    uint32_t                        GetWorkerConnectionLimit() const { return m_WorkerConnectionLimit; }
    uint32_t                        GetDistributableJobMemoryLimitMiB() const { return m_DistributableJobMemoryLimitMiB; }
    bool                            GetDisableDBMigration() const { return m_DisableDBMigration; }
    bool                            GetSandboxEnabled() const { return m_SandboxEnabled; }
    void                            SetSandboxEnabled( const bool sandboxEnabled );
    inline const AString &          GetSandboxExe() const { return m_SandboxExe; }
    void                            SetSandboxExe( const AString & path );
    const AString &                 GetAbsSandboxExe() const;
    inline const AString &          GetSandboxArgs() const { return m_SandboxArgs; }
    void                            SetSandboxArgs( const AString & args );
    inline const AString &          GetSandboxTmp() const { return m_SandboxTmp; }
    void                            SetSandboxTmp( const AString & path );
    const AString &                 GetObfuscatedSandboxTmp() const;
    void                            SetObfuscatedSandboxTmp( const AString & obfuscatedSandboxTmp );

private:
    void ProcessEnvironment( const Array< AString > & envStrings ) const;

    // Settings from environment variables
    AString             m_CachePathFromEnvVar;
    AString             m_CachePathMountPointFromEnvVar;

    // Exposed settings
    //friend class FunctionSettings;
    Array< AString  > m_Environment;
    AString           m_CachePath;
    AString           m_CachePathMountPoint;
    AString           m_CachePluginDLL;
    Array< AString  > m_Workers;
    uint32_t          m_WorkerConnectionLimit;
    uint32_t          m_DistributableJobMemoryLimitMiB;
    bool              m_DisableDBMigration; // TODO:C Remove this option some time after v0.99

    bool              m_SandboxEnabled;
    AString           m_SandboxExe;
    AString           m_SandboxArgs;
    AString           m_SandboxTmp;
    
    mutable AString   m_AbsSandboxExe;
    mutable AString   m_ObfuscatedSandboxTmp;
};

//------------------------------------------------------------------------------
