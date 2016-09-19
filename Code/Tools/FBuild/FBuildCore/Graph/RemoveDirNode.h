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
public:
    explicit RemoveDirNode( const AString & name,
                            const Dependencies & staticDeps,
                            const Dependencies & preBuildDeps );
    virtual ~RemoveDirNode();

    static inline Node::Type GetTypeS() { return Node::REMOVE_DIR_NODE; }
    virtual bool IsAFile() const override;

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;

private:
    virtual BuildResult DoBuild( Job * job ) override;
};

//------------------------------------------------------------------------------
