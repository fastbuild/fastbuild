// TestCompiler.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileStream.h"

// TestCompiler
//------------------------------------------------------------------------------
class TestCompiler : public FBuildTest
{
private:
    DECLARE_TESTS

    void BuildCompiler_Explicit() const;
    void BuildCompiler_Explicit_NoRebuild() const;
    void BuildCompiler_Implicit() const;
    void BuildCompiler_Implicit_NoRebuild() const;
    void ConflictingFiles1() const;
    void ConflictingFiles2() const;
    void ConflictingFiles3() const;
    void ConflictingFiles4() const;
    void CompilerExecutableAsDependency() const;
    void CompilerExecutableAsDependency_NoRebuild() const;
    void MultipleImplicitCompilers() const;

    void Parse( const char * fileName, bool expectFailure = false ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCompiler )
    REGISTER_TEST( BuildCompiler_Explicit )
    REGISTER_TEST( BuildCompiler_Explicit_NoRebuild )
    REGISTER_TEST( BuildCompiler_Implicit )
    REGISTER_TEST( BuildCompiler_Implicit_NoRebuild )
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
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Explicit/explicit.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Build a file genereated by a Compiler that we compiled
    TEST_ASSERT( fBuild.Build( AStackString<>( "Compiler" ) ) );

    // Save DB for use by NoRebuild test
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Explicit/explicit.fdb" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
}

// BuildCompiler_Explicit_NoRebuild
//------------------------------------------------------------------------------
void TestCompiler::BuildCompiler_Explicit_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Explicit/explicit.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TestCompiler/Explicit/explicit.fdb" ) );

    // Build a file genereated by a Compiler that we compiled
    TEST_ASSERT( fBuild.Build( AStackString<>( "Compiler" ) ) );

    // Check stats
    //              Seen,   Built,  Type
    CheckStatsNode( 1,      0,      Node::COMPILER_NODE );
}

// BuildCompiler_Implicit
//------------------------------------------------------------------------------
void TestCompiler::BuildCompiler_Implicit() const
{
    FBuildTestOptions options;
    options.m_ForceCleanBuild = true;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Implicit/implicit.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize() );

    // Build a file genereated by a Compiler that we compiled
    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

    // Save DB for use by NoRebuild test
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     1,      Node::COMPILER_NODE );
}

// BuildCompiler_Implicit_NoRebuild
//------------------------------------------------------------------------------
void TestCompiler::BuildCompiler_Implicit_NoRebuild() const
{
    FBuildTestOptions options;
    options.m_ConfigFile = "Tools/FBuild/FBuildTest/Data/TestCompiler/Implicit/implicit.bff";
    FBuild fBuild( options );
    TEST_ASSERT( fBuild.Initialize( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

    // Build a file genereated by a Compiler that we compiled
    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

    // Save DB for use by NoRebuild test
    TEST_ASSERT( fBuild.SaveDependencyGraph( "../tmp/Test/TestCompiler/Implicit/implicit.fdb" ) );

    // Check stats
    //               Seen,  Built,  Type
    CheckStatsNode ( 1,     0,      Node::COMPILER_NODE );
}

// ConflictingFiles1
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles1() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict1.bff", true ); // Expect failure
}

// ConflictingFiles2
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles2() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict2.bff", true ); // Expect failure
}

// ConflictingFiles3
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles3() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict3.bff", true ); // Expect failure
}

// ConflictingFiles4
//------------------------------------------------------------------------------
void TestCompiler::ConflictingFiles4() const
{
    Parse( "Tools/FBuild/FBuildTest/Data/TestCompiler/conflict4.bff", true ); // Expect failure
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
    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

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
    TEST_ASSERT( fBuild.Build( AStackString<>( "ObjectList" ) ) );

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

// Parse
//------------------------------------------------------------------------------
void TestCompiler::Parse( const char * fileName, bool expectFailure ) const
{
    FileStream f;
    TEST_ASSERT( f.Open( fileName, FileStream::READ_ONLY ) );
    uint32_t fileSize = (uint32_t)f.GetFileSize();
    AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
    mem.Get()[ fileSize ] = '\000'; // parser requires sentinel
    TEST_ASSERT( f.Read( mem.Get(), fileSize ) == fileSize );

    FBuild fBuild;
    NodeGraph ng;
    BFFParser p( ng );
    bool parseResult = p.Parse( mem.Get(), fileSize, fileName, 0, 0 );
    if ( expectFailure )
    {
        TEST_ASSERT( parseResult == false ); // Make sure it failed as expected
    }
    else
    {
        TEST_ASSERT( parseResult == true );
    }
}

//------------------------------------------------------------------------------
