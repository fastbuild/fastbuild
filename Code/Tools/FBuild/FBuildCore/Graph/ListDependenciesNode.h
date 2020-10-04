// ListDependenciesNode.h - a node that list dependencies of a target to a file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Function;

// ListDependenciesNode
//------------------------------------------------------------------------------
class ListDependenciesNode : public FileNode
{
    REFLECT_NODE_DECLARE( ListDependenciesNode )
public:
    explicit ListDependenciesNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * funcStartIter, const Function * function ) override;
    virtual ~ListDependenciesNode() override;

    static inline Node::Type GetTypeS() { return Node::LIST_DEPENDENCIES_NODE; }

private:
    virtual BuildResult DoBuild( Job * job ) override;

    void EmitOutputMessage() const;

    friend class FunctionListDependencies;
    AString             m_Source;
    AString             m_Dest;
    Array< AString >    m_Patterns;
    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
