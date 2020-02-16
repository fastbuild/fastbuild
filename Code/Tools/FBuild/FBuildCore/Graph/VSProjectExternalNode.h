// VSProjectExternalNode.h - a node that references an existing, external Visual Studio proj file of arbitrary type
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "VSProjectBaseNode.h"


// VSExternalProjectConfig
//  - config / platform tuples as found in the external project or provided in the BFF
//------------------------------------------------------------------------------
class VSExternalProjectConfig : public Struct
{
    REFLECT_STRUCT_DECLARE(VSExternalProjectConfig)
public:
    VSExternalProjectConfig() = default;
    explicit VSExternalProjectConfig(const VSExternalProjectConfig& otherConfig)
    {
        m_Config = otherConfig.m_Config;
        m_Platform = otherConfig.m_Platform;
    }

    AString             m_Platform;
    AString             m_Config;
};

// VSProjectExternalNode
//------------------------------------------------------------------------------
class VSProjectExternalNode : public VSProjectBaseNode
{
    REFLECT_NODE_DECLARE( VSProjectExternalNode )
public:
    VSProjectExternalNode();
    virtual bool Initialize( NodeGraph& nodeGraph, const BFFToken* iter, const Function* function ) override;
    virtual ~VSProjectExternalNode() override;

    static inline Node::Type GetTypeS() { return Node::VSPROJEXTERNAL_NODE; }

private:
    virtual BuildResult DoBuild( Job* job ) override;
    void CopyConfigs();

    Array< VSExternalProjectConfig > m_ProjectConfigs;
};

//------------------------------------------------------------------------------
