// CopyFileNode.h - a node that copies a single object
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Function;

// CopyFileNode
//------------------------------------------------------------------------------
class CopyFileNode : public FileNode
{
    REFLECT_NODE_DECLARE( CopyFileNode )
public:
    explicit CopyFileNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~CopyFileNode() override;

    static inline Node::Type GetTypeS() { return Node::COPY_FILE_NODE; }

    FileNode * GetSourceNode() const { return m_StaticDependencies[0].GetNode()->CastTo< FileNode >(); }

private:
    virtual BuildResult DoBuild( Job * job ) override;

    void EmitCopyMessage() const;

    friend class FunctionCopy;
    friend class CopyDirNode; // TODO: Remove
    AString             m_Source;
    AString             m_Dest;
    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
