// TestCompiler.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"

// TestCompiler
//------------------------------------------------------------------------------
class TestCompiler : public FBuildTest
{
private:
    DECLARE_TESTS

    void BuildCompiler_Explicit() const;
    void BuildCompiler_Implicit() const;
    void ConflictingFiles1() const;
    void ConflictingFiles2() const;
    void ConflictingFiles3() const;
    void ConflictingFiles4() const;
    void CompilerExecutableAsDependency() const;
    void CompilerExecutableAsDependency_NoRebuild() const;
    void MultipleImplicitCompilers() const;

    uint64_t GetToolId( const FBuildForTest & fBuild ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCompiler )
    REGISTER_TEST( BuildCompiler_Explicit )
    REGISTER_TEST( BuildCompiler_Implicit )
    REGISTER_TEST( ConflictingFiles1 )
    REGISTER_TEST( ConflictingFiles2 )
    REGISTER_TEST( ConflictingFiles3 )
    REGISTER_TEST( ConflictingFiles4 )
    REGISTER_TEST( CompilerExecutableAsDependency )
    REGISTER_TEST( CompilerExecutableAsDependency_NoRebuild )
    REGISTER_TEST( MultipleImplicitCompilers )
REGISTER_TESTS_END

// BuildCompiler_Explicit
//------------------------------------------------------------------------------
void TestCompiler::BuildCompiler_Explicit() const
{
    uint64_t toolIdA;
    uint64_t toolIdB;
    uint64_t toolIdC;

    // Build
    {
        FBuildTestOptions options;
        options.m_ForceCleanBuild = true;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Explicit/explicit.bff";
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Build a file genereated by a Compiler that we compiled
        TEST_ASSERT( fBuild.Build( "Compiler" ) );

        // Save DB for use by NoRebuild test
        TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Explicit/explicit.fdb" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );

        // Get the ToolId
        toolIdA = GetToolId( fBuild );
    }

    // Check no rebuild
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Explicit/explicit.bff";
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TestCompiler/Explicit/explicit.fdb" ) );

        // Build a file genereated by a Compiler that we compiled
        TEST_ASSERT( fBuild.Build( "Compiler" ) );

        // Check stats
        //              Seen,   Built,  Type
        CheckStatsNode( 1,      0,      Node::COMPILER_NODE );

        // Get the ToolId
        toolIdB = GetToolId( fBuild );
    }

    // Check no rebuild after Migration
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Explicit/explicit.bff";
        options.m_ForceDBMigration_Debug = true;
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TestCompiler/Explicit/explicit.fdb" ) );

        // Build a file genereated by a Compiler that we compiled
        TEST_ASSERT( fBuild.Build( "Compiler" ) );

        // Check stats
        //              Seen,   Built,  Type
        CheckStatsNode( 1,      0,      Node::COMPILER_NODE );

        // Get the ToolId
        toolIdC = GetToolId( fBuild );
    }

    // ToolId should be identical for each case
    TEST_ASSERT( toolIdA == toolIdB );
    TEST_ASSERT( toolIdA == toolIdC );
}

// BuildCompiler_Implicit
//------------------------------------------------------------------------------
void TestCompiler::BuildCompiler_Implicit() const
{
    uint64_t toolIdA;
    uint64_t toolIdB;
    uint64_t toolIdC;
    AStackString<> compilerNodeName;

    // Implicit Definition
    {
        FBuildTestOptions options;
        options.m_ForceCleanBuild = true;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Implicit/implicit.bff";
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Build a file genereated by a Compiler that we compiled
        TEST_ASSERT( fBuild.Build( "ObjectList" ) );

        // Save DB for use by next test
        TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

        // Ensure node was implicitly created
        Array< const Node * > compilerNodes;
        fBuild.GetNodesOfType( Node::COMPILER_NODE, compilerNodes );
        TEST_ASSERT( compilerNodes.GetSize() == 1 );
        compilerNodeName = compilerNodes[ 0 ]->GetName();

        // Check stats
        // - Since we have no object files, the implicitly created Compiler node
        //   will not be built
        //               Seen,  Built,  Type
        CheckStatsNode ( 0,     0,      Node::COMPILER_NODE );
    }

    // Build
    {
        FBuildTestOptions options;
        options.m_ForceCleanBuild = true;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Implicit/implicit.bff";
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize() );

        // Build the compiler so we can test it's behavior
        TEST_ASSERT( fBuild.Build( compilerNodeName ) );

        // Ensure node was implicitly created
        Array< const Node * > compilerNodes;
        fBuild.GetNodesOfType( Node::COMPILER_NODE, compilerNodes );
        TEST_ASSERT( compilerNodes.GetSize() == 1 );

        // Save DB for use by next test
        TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );

        // Get the ToolId
        toolIdA = GetToolId( fBuild );
    }

    // Check No Rebuild
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Implicit/implicit.bff";
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

        // Build a file genereated by a Compiler that we compiled
        TEST_ASSERT( fBuild.Build( compilerNodeName ) );

        // Save DB for use by NoRebuild test
        TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );

        // Get the ToolId
        toolIdB = GetToolId( fBuild );
    }

    // Check no rebuild after Migration
    {
        FBuildTestOptions options;
        options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Implicit/implicit.bff";
        options.m_ForceDBMigration_Debug = true;
        FBuildForTest fBuild( options );
        TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

        // Build a file genereated by a Compiler that we compiled
        TEST_ASSERT( fBuild.Build( compilerNodeName ) );

        // Save DB for use by NoRebuild test
        TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

        // Check stats
        //               Seen,  Built,  Type
        CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );

        // Get the ToolId
        toolIdC = GetToolId( fBuild );
    }

    // ToolId should be identical for each case
    TEST_ASSERT( toolIdA == toolIdB );
    TEST_ASSERT( toolIdA == toolIdC );
}

// ConflictingFiles1
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles1() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict1.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1100" ) );
}

// ConflictingFiles2
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles2() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict2.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1100" ) );
}

// ConflictingFiles3
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles3() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict3.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1100" ) );
}

// ConflictingFiles4
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles4() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict4.bff", true ); // Expect failure
    TEST_ASSERT( GetRecordedOutput().Find( "Error #1100" ) );
}

// CompilerExecutableAsDependency
//------------------------------------------------------------------------------
void TestCompiler::CompilerExecutableAsDependency() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/compile.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Build a file genereated by a Compiler that we compiled
    TEST_ASSERT( fBuild.Build( "ObjectList" ) );

    // Save DB for use by NoRebuild test
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/executableasdependency.fdb" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
}

// CompilerExecutableAsDependency_NoRebuild
//------------------------------------------------------------------------------
void TestCompiler::CompilerExecutableAsDependency_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/compile.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TestCompiler/executableasdependency.fdb" ) );

    // Build a file genereated by a Compiler that we compiled
    TEST_ASSERT( fBuild.Build( "ObjectList" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
}

// MultipleImplicitCompilers
//------------------------------------------------------------------------------
void TestCompiler::MultipleImplicitCompilers() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/multipleimplicitcompilers.bff" );
}

// GetToolId
//------------------------------------------------------------------------------
uint64_t TestCompiler::GetToolId( const FBuildForTest & fBuild ) const
{
    Array<const Node *> nodes;
    fBuild.GetNodesOfType( Node::COMPILER_NODE, nodes );
    TEST_ASSERT( nodes.GetSize() == 1 );
    const uint64_t toolId = nodes[ 0 ]->CastTo<CompilerNode>()->GetManifest().GetToolId();
    TEST_ASSERT( toolId );
    return toolId;
}

//------------------------------------------------------------------------------
