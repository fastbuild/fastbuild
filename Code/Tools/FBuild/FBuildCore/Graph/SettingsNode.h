// SettingsNode.h - Global configuration settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// SettingsNode
//------------------------------------------------------------------------------
class SettingsNode : public Node
{
    REFLECT_NODE_DECLARE( SettingsNode )
public:
    explicit SettingsNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function ) override;
    virtual ~SettingsNode();

    static inline Node::Type GetTypeS() { return Node::SETTINGS_NODE; }

    virtual bool IsAFile() const override;

    // Access to settings
    const AString &        GetCachePath() const;
    const AString &        GetCachePathMountPoint() const;
    const AString &        GetCachePluginDLL() const;
    inline const Array< AString > & GetWorkerList() const { return m_Workers; }
    int32_t                GetWorkerListRefreshLimitSec() const { return m_WorkerListRefreshLimitSec; }
    int32_t                GetWorkerConnectionRetryLimitSec() const { return m_WorkerConnectionRetryLimitSec; }
    uint32_t               GetWorkerConnectionLimit() const { return m_WorkerConnectionLimit; }
    uint32_t               GetDistributableJobMemoryLimitMiB() const { return m_DistributableJobMemoryLimitMiB; }
    bool                   GetAllowDBMigration_Experimental() const { return m_AllowDBMigration_Experimental; }

private:
    //virtual BuildResult DoBuild( Job * job ) override;

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
    bool              m_AllowDBMigration_Experimental;

    int32_t           m_WorkerListRefreshLimitSec;
    int32_t           m_WorkerConnectionRetryLimitSec;
};

//------------------------------------------------------------------------------
