// DLLNode.h - builds a dll
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "LinkerNode.h"

// Forward Declarations
//------------------------------------------------------------------------------

// DLLNode
//------------------------------------------------------------------------------
class DLLNode : public LinkerNode
{
    REFLECT_NODE_DECLARE( DLLNode )
public:
    explicit DLLNode();
    virtual ~DLLNode();

    void GetImportLibName( AString & importLibName ) const;

    static inline Node::Type GetTypeS() { return Node::DLL_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
};

//------------------------------------------------------------------------------
