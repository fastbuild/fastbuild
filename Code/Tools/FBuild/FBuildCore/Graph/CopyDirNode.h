// CopyDirNode.h - a node that copies one or more directories
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;
class NodeGraph;

// CopyDirNode
//------------------------------------------------------------------------------
class CopyDirNode : public Node
{
    REFLECT_NODE_DECLARE( CopyDirNode )
public:
    explicit CopyDirNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~CopyDirNode();

    static inline Node::Type GetTypeS() { return Node::COPY_DIR_NODE; }
    virtual bool IsAFile() const override;

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;

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
