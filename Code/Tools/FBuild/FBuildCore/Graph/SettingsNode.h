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
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~SettingsNode();

    static inline Node::Type GetTypeS() { return Node::SETTINGS_NODE; }

    virtual bool IsAFile() const override;

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;

    // Access to settings
    const AString &                     GetCachePath() const;
    const AString &                     GetCachePluginDLL() const;
    inline const Array< AString > &     GetWorkerList() const { return m_Workers; }
    uint32_t                            GetWorkerConnectionLimit() const { return m_WorkerConnectionLimit; }
    uint32_t                            GetDistributableJobMemoryLimitMiB() const { return m_DistributableJobMemoryLimitMiB; }

private:
    //virtual BuildResult DoBuild( Job * job ) override;

    void ProcessEnvironment( const Array< AString > & envStrings ) const;

    // Settings from environment variables
    AString             m_CachePathFromEnvVar;

    // Exposed settings
    //friend class FunctionSettings;
    Array< AString  >   m_Environment;
    AString             m_CachePath;
    AString             m_CachePluginDLL;
    Array< AString  >   m_Workers;
    uint32_t            m_WorkerConnectionLimit;
    uint32_t            m_DistributableJobMemoryLimitMiB;
};

//------------------------------------------------------------------------------
