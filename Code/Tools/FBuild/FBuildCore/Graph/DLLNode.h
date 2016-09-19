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
public:
    explicit DLLNode( const AString & linkerOutputName,
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
    virtual ~DLLNode();

    void GetImportLibName( AString & importLibName ) const;

    static inline Node::Type GetTypeS() { return Node::DLL_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
private:
};

//------------------------------------------------------------------------------
