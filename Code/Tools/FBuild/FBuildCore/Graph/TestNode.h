// TestNode.h - Run a Test
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ExecNode.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFIterator;
class Function;

// TestNode
//------------------------------------------------------------------------------
class TestNode : public FileNode
{
    REFLECT_NODE_DECLARE( TestNode )
public:
    TestNode();

    virtual bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function ) override;
    virtual ~TestNode() override;

    static inline Node::Type GetTypeS() { return Node::TEST_NODE; }
    virtual const Tags & GetRequiredWorkerTags() const override;

    inline const Node* GetTestExecutable() const { return m_StaticDependencies[0].GetNode(); }
    const char * GetEnvironmentString() const;

private:
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;

    void EmitCompilationMessage( const AString & workingDir, const AString & testExe ) const;

    AString          m_TestExecutable;
    Array< AString > m_TestInput;
    Array< AString > m_TestInputPath;
    Array< AString > m_TestInputPattern;
    Array< AString > m_TestInputExcludePath;
    Array< AString > m_TestInputExcludedFiles;
    Array< AString > m_TestInputExcludePattern;
    AString          m_TestArguments;
    AString          m_TestWorkingDir;
    uint32_t         m_TestTimeOut;
    bool             m_TestAlwaysShowOutput;
    bool             m_TestInputPathRecurse;
    Array< AString > m_PreBuildDependencyNames;
    Array< AString > m_Environment;
    Array< AString > m_RequiredWorkerTagStrings;

    // Internal State
    uint32_t         m_NumTestInputFiles;
    mutable const char * m_EnvironmentString;
    mutable Tags     m_RequiredWorkerTags;
};

//------------------------------------------------------------------------------
