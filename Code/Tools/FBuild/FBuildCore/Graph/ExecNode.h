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
public:
    explicit ExecNode( const AString & dstFileName,
                        const Dependencies & inputFiles,
                        FileNode * executable,
                        const AString & arguments,
                        const AString & workingDir,
                        int32_t expectedReturnCode,
                        const Dependencies & preBuildDependencies,
                        bool useStdOutAsOutput,
                        bool isGenerator = false );
    virtual ~ExecNode();

    static inline Node::Type GetTypeS() { return Node::EXEC_NODE; }

    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );
    virtual void Save( IOStream & stream ) const override;
    virtual bool IsGenerator() const override;
private:
    virtual BuildResult DoBuild( Job * job ) override;

    void GetFullArgs(AString & fullArgs) const;
    void GetInputFiles(AString & fullArgs, const AString & pre, const AString & post) const;

    void EmitCompilationMessage( const AString & args ) const;

    Dependencies m_InputFiles;
    FileNode * m_Executable;
    AString     m_Arguments;
    AString     m_WorkingDir;
    int32_t     m_ExpectedReturnCode;
    bool        m_UseStdOutAsOutput;
    bool        m_IsGenerator;
};

//------------------------------------------------------------------------------
