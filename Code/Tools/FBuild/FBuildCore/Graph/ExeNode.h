// ExeNode.h - builds an exe
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "LinkerNode.h"

// Forward Declarations
//------------------------------------------------------------------------------

// ExeNode
//------------------------------------------------------------------------------
class ExeNode : public LinkerNode
{
    REFLECT_NODE_DECLARE( ExeNode )
public:
    explicit ExeNode();
    virtual ~ExeNode() override;

    static inline Node::Type GetTypeS() { return Node::EXE_NODE; }
};

//------------------------------------------------------------------------------
