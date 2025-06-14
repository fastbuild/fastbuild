// SettingsNode.h - Global configuration settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ConcurrencyGroup;
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

    static Node::Type GetTypeS() { return Node::SETTINGS_NODE; }

    virtual bool IsAFile() const override;

    // Access to settings
    const AString & GetCachePath() const;
    const AString & GetCachePathMountPoint() const;
    const AString & GetCachePluginDLL() const;
    const AString & GetCachePluginDLLConfig() const;
    const Array<AString> & GetWorkerList() const { return m_Workers; }
    uint32_t GetWorkerConnectionLimit() const { return m_WorkerConnectionLimit; }
    uint32_t GetDistributableJobMemoryLimitMiB() const { return m_DistributableJobMemoryLimitMiB; }
    const ConcurrencyGroup * GetConcurrencyGroup( const AString & groupName ) const;
    const ConcurrencyGroup & GetConcurrencyGroup( uint8_t index ) const;

    inline static const uint32_t kMaxConcurrencyGroups = 16;

private:
    void ProcessEnvironment( const Array<AString> & envStrings ) const;
    [[nodiscard]] bool InitializeConcurrencyGroups( const BFFToken * iter,
                                                    const Function * function );

    // Settings from environment variables
    AString m_CachePathFromEnvVar;
    AString m_CachePathMountPointFromEnvVar;

    // Exposed settings
    Array<AString> m_Environment;
    AString m_CachePath;
    AString m_CachePathMountPoint;
    AString m_CachePluginDLL;
    AString m_CachePluginDLLConfig;
    Array<AString> m_Workers;
    uint32_t m_WorkerConnectionLimit;
    uint32_t m_DistributableJobMemoryLimitMiB;
    Array<ConcurrencyGroup> m_ConcurrencyGroups;
};

//------------------------------------------------------------------------------
class ConcurrencyGroup
{
    REFLECT_STRUCT_DECLARE( ConcurrencyGroup )
public:
    // General accessors
    const AString & GetName() const { return m_ConcurrencyGroupName; }
    uint32_t GetLimit() const { return m_Limit; }
    uint8_t GetIndex() const { return m_Index; }

    inline static const uint32_t eUnlimited = 0xFFFFFFFF;

protected:
    friend SettingsNode;

    // SettingsNode initializes via these methods
    void SetIndex( uint8_t index ) { m_Index = index; }
    void SetLimit( uint32_t limit ) { m_Limit = limit; }
    uint32_t GetJobBasedLimit() const { return m_ConcurrencyLimit; }
    uint32_t GetMemoryBasedLimit() const { return m_ConcurrencyPerJobMiB; }

    // Exposed
    AString m_ConcurrencyGroupName;
    uint32_t m_ConcurrencyLimit = 0; // No limit by default
    uint32_t m_ConcurrencyPerJobMiB = 0; // No limit by default

    // Internal
    uint8_t m_Index = 0;
    uint32_t m_Limit = 0; // Limit evaluated from exposed settings
};

//------------------------------------------------------------------------------
