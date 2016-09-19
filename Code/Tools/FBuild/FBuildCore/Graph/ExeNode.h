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
public:
    explicit ExeNode( const AString & linkerOutputName,
                      const Dependencies & inputLibraries,
                      const Dependencies & otherLibraries,
                      const AString & linkerType,
                      const AString & linker,
                      const AString & linkerArgs,
                      uint32_t flags,
                      const Dependencies & assemblyResources,
                      const AString & importLibName,
                      Node * linkerStampExe,
                      const AString & linkerStampExeArgs );
    virtual ~ExeNode();

    static inline Node::Type GetTypeS() { return Node::EXE_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
private:
};

//------------------------------------------------------------------------------
