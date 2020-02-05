// AliasNode.h - a node that groups together several targets under a new target
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Function;

// AliasNode
//------------------------------------------------------------------------------
class AliasNode : public Node
{
    REFLECT_NODE_DECLARE( AliasNode )
public:
    explicit AliasNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~AliasNode() override;

    static inline Node::Type GetTypeS() { return Node::ALIAS_NODE; }

    virtual bool IsAFile() const override { return false; }

    inline const Dependencies & GetAliasedNodes() const { return m_StaticDependencies; }

private:
    virtual BuildResult DoBuild( Job * job ) override;

    Array< AString > m_Targets;
};

//------------------------------------------------------------------------------
