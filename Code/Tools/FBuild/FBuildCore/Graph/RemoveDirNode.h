// RemoveDirNode.h - a node that removes one or more directories
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------

// RemoveDirNode
//------------------------------------------------------------------------------
class RemoveDirNode : public Node
{
    REFLECT_NODE_DECLARE( RemoveDirNode )
public:
    explicit RemoveDirNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~RemoveDirNode() override;

    static inline Node::Type GetTypeS() { return Node::REMOVE_DIR_NODE; }
    virtual bool IsAFile() const override;

private:
    virtual BuildResult DoBuild( Job * job ) override;

    // Exposed Properties
    Array< AString >    m_RemovePaths;
    bool                m_RemovePathsRecurse;
    Array< AString >    m_RemovePatterns;
    Array< AString >    m_RemoveExcludePaths;
    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
