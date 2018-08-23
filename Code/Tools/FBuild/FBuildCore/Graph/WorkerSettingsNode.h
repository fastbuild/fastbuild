// WorkerSettingsNode.h - Worker configuration settings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

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

private:
    // Exposed settings
    //friend class FunctionWorkerSettings;
    uint32_t         m_FileVersion;
    AString          m_WorkModeString;
    mutable uint8_t  m_WorkModeEnum;
    uint32_t         m_NumCPUsToUse;
    bool             m_StartMinimized;
};

//------------------------------------------------------------------------------
