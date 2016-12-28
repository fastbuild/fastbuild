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
    bool Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function );
    virtual ~TestNode();

    static inline Node::Type GetTypeS() { return Node::TEST_NODE; }

    virtual void Save( IOStream & stream ) const override;
    static Node * Load( NodeGraph & nodeGraph, IOStream & stream );

    inline const Node* GetTestExecutable() const { return m_StaticDependencies[0].GetNode(); }
private:
    virtual BuildResult DoBuild( Job * job ) override;

    void EmitCompilationMessage( const char * workingDir ) const;

    AString     m_TestExecutable;
    AString     m_TestArguments;
    AString     m_TestWorkingDir;
    uint32_t    m_TestTimeOut;
    bool        m_TestAlwaysShowOutput;
};

//------------------------------------------------------------------------------
