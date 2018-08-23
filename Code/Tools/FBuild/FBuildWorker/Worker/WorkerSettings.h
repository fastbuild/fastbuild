// WorkerSettings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Singleton.h"
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

    void Load();
    void Save();
private:
    NodeGraph * m_NodeGraph;
    WorkerSettingsNode * m_WorkerSettingsNode;
    bool m_WorkerSettingsNodeOwned;
};

//------------------------------------------------------------------------------
