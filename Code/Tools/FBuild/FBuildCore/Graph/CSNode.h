// CSNode.h - a node that builds a C# assembly
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class Function;

// CSNode
//------------------------------------------------------------------------------
class CSNode : public FileNode
{
    REFLECT_NODE_DECLARE( CSNode )
public:
    explicit CSNode();
    virtual bool Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function ) override;
    virtual ~CSNode() override;

    static inline Node::Type GetTypeS() { return Node::CS_NODE; }

private:
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;

    CompilerNode * GetCompiler() const;

    void EmitCompilationMessage( const Args & fullArgs ) const;

    bool BuildArgs( Args & fullArgs ) const;
    void GetInputFiles( Args & fullArgs, const AString & pre, const AString & post ) const;
    void GetExtraRefs( Args & fullArgs, const AString & pre, const AString & post ) const;

    // Exposed Properties
    AString             m_Compiler;
    AString             m_CompilerOptions;
    AString             m_CompilerOutput;
    Array< AString >    m_CompilerInputPath;
    bool                m_CompilerInputPathRecurse;
    Array< AString >    m_CompilerInputPattern;
    Array< AString >    m_CompilerInputExcludePath;
    Array< AString >    m_CompilerInputExcludedFiles;
    Array< AString >    m_CompilerInputExcludePattern;
    Array< AString >    m_CompilerInputFiles;
    Array< AString >    m_CompilerReferences;
    Array< AString >    m_PreBuildDependencyNames;

    // Internal State
    uint32_t            m_NumCompilerInputFiles;
    uint32_t            m_NumCompilerReferences;
};

//------------------------------------------------------------------------------
