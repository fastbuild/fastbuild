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

    [[nodiscard]] bool FileDelete( const AString & file ) const;
    [[nodiscard]] bool DirectoryDelete( const AString & dir ) const;

    // Exposed Properties
    Array< AString >    m_RemovePaths;
    bool                m_RemovePathsRecurse = true;
    bool                m_RemoveDirs = true;
    bool                m_RemoveRootDir = true;
    Array< AString >    m_RemovePatterns;
    Array< AString >    m_RemoveExcludePaths;
    Array< AString >    m_RemoveExcludeFiles;
    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
