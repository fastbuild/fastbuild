// ExecNode
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "FileNode.h"
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------

// ExecNode
//------------------------------------------------------------------------------
class ExecNode : public FileNode
{
    REFLECT_NODE_DECLARE( ExecNode )
public:
    explicit ExecNode();
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~ExecNode();

    static inline Node::Type GetTypeS() { return Node::EXEC_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;
private:
    virtual BuildResult DoBuild( Job * job ) override;

    const FileNode * GetExecutable() const { return m_StaticDependencies[0].GetNode()->CastTo< FileNode >(); }
    void GetFullArgs(AString & fullArgs) const;
    void GetInputFiles(AString & fullArgs, const AString & pre, const AString & post) const;

    void EmitCompilationMessage( const AString & args ) const;

    // Exposed Properties
    AString             m_ExecExecutable;
    Array< AString >    m_ExecInput;
    Array< AString >    m_ExecInputPath;
    Array< AString >    m_ExecInputPattern;
    Array< AString >    m_ExecInputExcludePath;
    Array< AString >    m_ExecInputExcludedFiles;
    Array< AString >    m_ExecInputExcludePattern;
    AString             m_ExecArguments;
    AString             m_ExecWorkingDir;
    int32_t             m_ExecReturnCode;
    bool                m_ExecUseStdOutAsOutput;
    bool                m_ExecInputPathRecurse;
    Array< AString >    m_PreBuildDependencyNames;
};

//------------------------------------------------------------------------------
