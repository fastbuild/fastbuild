// FileNode.h - a node that tracks a single file
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// FileNode
//------------------------------------------------------------------------------
class FileNode : public Node
{
public:
    explicit FileNode( const AString & fileName, uint8_t controlFlags );
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * funcStartIter, const Function * function ) override;
    virtual ~FileNode() override;

    static inline Node::Type GetTypeS() { return Node::FILE_NODE; }

    virtual bool IsAFile() const override { return true; }

    static void HandleWarningsMSVC( Job * job, const AString & name, const AString & data );
    static void HandleWarningsClangCl( Job * job, const AString & name, const AString & data );
    static void HandleWarningsClangGCC( Job * job, const AString & name, const AString & data );
protected:
    friend class ObjectNode;
    virtual BuildResult DoBuild( Job * job ) override;

    static void DumpOutput( Job * job, const AString & name, const AString & data, bool treatAsWarnings = false );
    static void HandleWarnings( Job * job, const AString & name, const AString & data, const char * warningString );

    friend class Client;
};

//------------------------------------------------------------------------------
