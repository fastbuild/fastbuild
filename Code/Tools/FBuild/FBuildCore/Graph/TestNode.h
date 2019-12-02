// TestNode.h - Run a Test
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "ExecNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"
#include "Core/Time/Timer.h"

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
    // simplified remote constructor
    explicit TestNode( const AString & objectName, const AString & testExecutable,
        const AString & testArguments, const AString & testWorkingDir, const uint32_t testTimeout, 
        const bool testAlwaysShowOutput, const bool allowDistribution,
        const Array< AString > & customEnvironmentVariables );

    virtual bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function ) override;
    virtual ~TestNode() override;

    static inline Node::Type GetTypeS() { return Node::TEST_NODE; }

    virtual void SaveRemote( IOStream & stream ) const override;
    static Node * LoadRemote( IOStream & stream );

    inline const ToolManifest & GetManifest() const { return m_Manifest; }

    inline const Node* GetTestExecutable() const { return m_StaticDependencies[0].GetNode(); }
    const char * GetEnvironmentString() const;

private:
    virtual bool DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean ) override;
    virtual BuildResult DoBuild( Job * job ) override;
    virtual BuildResult DoBuild2( Job * job, bool racingRemoteJob ) override;
    Node::BuildResult DoBuildCommon( Job * job,
        bool stealingRemoteJob, bool racingRemoteJob);
    virtual void Migrate( const Node & oldNode ) override;

    void EmitCompilationMessage( bool stealingRemoteJob, bool racingRemoteJob,
        bool isRemote, const AString & workingDir, const AString & testExe ) const;

    AString             m_TestExecutable;
    Array< AString >    m_ExtraFiles;
    Array< AString >    m_TestInput;
    Array< AString >    m_TestInputPath;
    Array< AString >    m_TestInputPattern;
    Array< AString >    m_TestInputExcludePath;
    Array< AString >    m_TestInputExcludedFiles;
    Array< AString >    m_TestInputExcludePattern;
    AString             m_TestArguments;
    AString             m_TestWorkingDir;
    uint32_t            m_TestTimeOut;
    bool                m_TestAlwaysShowOutput;
    Array< AString >    m_PreBuildDependencyNames;
    Array< AString >    m_Environment;
    bool                m_AllowDistribution;
    AString             m_ExecutableRootPath;
    Array< AString >    m_CustomEnvironmentVariables;
    bool                m_DeleteRemoteFilesWhenDone;

    // Internal State
    bool                m_TestInputPathRecurse;
    uint32_t            m_NumTestInputFiles;
    mutable const char * m_EnvironmentString;
    ToolManifest        m_Manifest;
};

//------------------------------------------------------------------------------
