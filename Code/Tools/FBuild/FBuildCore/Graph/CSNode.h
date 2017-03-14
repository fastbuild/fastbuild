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
class BFFIterator;
class Function;

// CSNode
//------------------------------------------------------------------------------
class CSNode : public FileNode
{
    REFLECT_NODE_DECLARE( CSNode )
public:
    explicit CSNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~CSNode();

    static inline Node::Type GetTypeS() { return Node::CS_NODE; }

    virtual void Save( IOStream & stream ) const override;
    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
private:
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;

    inline Node * GetCompiler() const { return m_StaticDependencies[ 0 ].GetNode(); }

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
