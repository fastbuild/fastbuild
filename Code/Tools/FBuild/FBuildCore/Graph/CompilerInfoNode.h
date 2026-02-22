// CompilerInfoNode.h - Store info queries from a compiler
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class Function;

//------------------------------------------------------------------------------
class CompilerInfoNode : public Node
{
    REFLECT_NODE_DECLARE( CompilerInfoNode )
public:
    CompilerInfoNode();
    virtual ~CompilerInfoNode() override;

    static Node::Type GetTypeS() { return Node::COMPILER_INFO_NODE; }

    const Array<AString> & GetBuiltInIncludes() const { return m_BuiltinIncludePaths; }

private:
    virtual bool IsAFile() const override;
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual Node::BuildResult DoBuild( Job * job ) override;

    void EmitCompilationMessage( const AString & args ) const;

    friend class ObjectListNode;

    // Settings/Configuration
    CompilerNode * m_Compiler = nullptr;
    bool m_NoStdInc = false;
    bool m_NoStdIncPP = false;

    // Info obtained during querying
    Array<AString> m_BuiltinIncludePaths;
};

//------------------------------------------------------------------------------
