// CopyDirNode.h - a node that copies one or more directories
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Function;
class NodeGraph;

// CopyDirNode
//------------------------------------------------------------------------------
class CopyDirNode : public Node
{
    REFLECT_NODE_DECLARE( CopyDirNode )
public:
    explicit CopyDirNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~CopyDirNode() override;

    static inline Node::Type GetTypeS() { return Node::COPY_DIR_NODE; }
    virtual bool IsAFile() const override;

private:
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;

    // Exposed Properties
    Array< AString >    m_SourcePaths;
    AString             m_Dest;
    Array< AString >    m_SourcePathsPattern;
    Array< AString >    m_SourceExcludePaths;
    bool                m_SourcePathsRecurse = true;

    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
