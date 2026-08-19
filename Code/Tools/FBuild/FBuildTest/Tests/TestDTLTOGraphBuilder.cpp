// TestDTLTOGraphBuilder.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/DTLTOData.h"
#include "Tools/FBuild/FBuildCore/Helpers/DTLTOGraphBuilder.h"
#include "Tools/FBuild/FBuildCore/Helpers/DTLTOJsonParser.h"

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Reflection/ReflectionInfo.h"
#include "Core/Strings/AStackString.h"

// TestDTLTOGraphBuilder
//------------------------------------------------------------------------------
TEST_GROUP( TestDTLTOGraphBuilder, FBuildTest )
{
};

// FindObjectList
//------------------------------------------------------------------------------
static Node * FindObjectList( NodeGraph & nodeGraph, const char * output )
{
    AStackString<> cleanOutput;
    NodeGraph::CleanPath( AStackString<>( output ), cleanOutput );
    AStackString<> listName;
    listName.Format( "DTLTO-List:%s", cleanOutput.Get() );
    return nodeGraph.FindNode( listName );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, BuildSynthesizesGraph )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    {
        DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
        job.m_Args.EmplaceBack( "a.obj" );
        job.m_Outputs.EmplaceBack( "a.o" );
    }
    {
        DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
        job.m_Args.EmplaceBack( "b.obj" );
        job.m_Outputs.EmplaceBack( "b.o" );
    }

    DTLTOGraphBuilder builder( nodeGraph );
    const Node * alias = builder.BuildGraph( data, AStackString<>( "dtlto-all" ) );

    TEST_ASSERT( alias );
    TEST_ASSERT( alias->GetType() == Node::ALIAS_NODE );
    TEST_ASSERT( alias->GetStaticDependencies().GetSize() == 2 ); // 2 jobs

    Node * compiler = nodeGraph.FindNode( AStackString<>( "Compiler-DTLTO" ) );
    TEST_ASSERT( compiler );
    TEST_ASSERT( compiler->GetType() == Node::COMPILER_NODE );

    Node * job1Node = FindObjectList( nodeGraph, "a.o" );
    TEST_ASSERT( job1Node );
    TEST_ASSERT( job1Node->GetType() == Node::OBJECT_LIST_NODE );

    Node * job2Node = FindObjectList( nodeGraph, "b.o" );
    TEST_ASSERT( job2Node );
    TEST_ASSERT( job2Node->GetType() == Node::OBJECT_LIST_NODE );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, JobCompilerOptions )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "a.obj" );
    job.m_Args.EmplaceBack( "-jobflag" );
    job.m_Outputs.EmplaceBack( "a.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    Node * jobNode = FindObjectList( nodeGraph, "a.o" );
    TEST_ASSERT( jobNode );

    AStackString<> options;
    TEST_ASSERT( jobNode->GetReflectionInfoV()->GetProperty( jobNode, "CompilerOptions", &options ) );
    TEST_ASSERT( options == "-jobflag -D_FASTBUILD_DTLTO_INPUT=\"%1\" \"a.obj\" -o \"%2\"" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, RejectsMissingCompiler )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Outputs.EmplaceBack( "out.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    const Node * alias = builder.BuildGraph( data, AStackString<>( "dtlto-all" ) );
    TEST_ASSERT( alias == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: no compiler specified" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, RejectsEmptyJobs )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );

    DTLTOGraphBuilder builder( nodeGraph );
    const Node * alias = builder.BuildGraph( data, AStackString<>( "dtlto-all" ) );
    TEST_ASSERT( alias == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "DTLTO: no jobs to build" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, RejectsDuplicateTarget )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "app_main.c.obj" );
    job.m_Outputs.EmplaceBack( "app_main.c.1.native.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    const Node * alias = builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ); // duplicate
    TEST_ASSERT( alias == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "already exists" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, BuildFromParsedJson )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    AStackString<> json( R"({
        "common": {
            "linker_output": "app.exe",
            "args": [ "clang.exe", "-O2" ],
            "inputs": []
        },
        "jobs": [
            {
                "args": [ "a.obj", "-fthinlto-index=a.thinlto.bc", "-o", "a.o" ],
                "inputs": [ "a.obj", "a.thinlto.bc" ],
                "outputs": [ "a.o" ]
            }
        ]
    })" );

    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() );

    DTLTOGraphBuilder builder( nodeGraph );
    const Node * alias = builder.BuildGraph( parser.GetData(), AStackString<>( "dtlto-all" ) );
    TEST_ASSERT( alias );
    TEST_ASSERT( alias->GetType() == Node::ALIAS_NODE );
    TEST_ASSERT( alias->GetStaticDependencies().GetSize() == 1 ); // 1 job

    Node * jobNode = FindObjectList( nodeGraph, "a.o" );
    TEST_ASSERT( jobNode );
    TEST_ASSERT( jobNode->GetType() == Node::OBJECT_LIST_NODE );

    const ReflectionInfo * ri = jobNode->GetReflectionInfoV();

    AStackString<> compiler;
    TEST_ASSERT( ri->GetProperty( jobNode, "Compiler", &compiler ) );
    TEST_ASSERT( compiler == "Compiler-DTLTO" );

    AStackString<> options;
    TEST_ASSERT( ri->GetProperty( jobNode, "CompilerOptions", &options ) );
    TEST_ASSERT( options == "-O2 -fthinlto-index=a.thinlto.bc -D_FASTBUILD_DTLTO_INPUT=\"%1\" \"a.obj\" -o \"%2\"" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, ComputesOutputNaming )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "app_main.c.obj" );
    job.m_Outputs.EmplaceBack( "app_main.c.1.25872.native.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    Node * jobNode = FindObjectList( nodeGraph, "app_main.c.1.25872.native.o" );
    TEST_ASSERT( jobNode );
    const ReflectionInfo * ri = jobNode->GetReflectionInfoV();

    AStackString<> path;
    AStackString<> ext;
    TEST_ASSERT( ri->GetProperty( jobNode, "CompilerOutputPath", &path ) );
    TEST_ASSERT( ri->GetProperty( jobNode, "CompilerOutputExtension", &ext ) );
    TEST_ASSERT( ext == ".1.25872.native.o" );

    // Reconstruct the object name ObjectList will emit (path + <input base> + ext) and
    // check it equals the desired output.
    AStackString<> cleanOutput;
    NodeGraph::CleanPath( AStackString<>( "app_main.c.1.25872.native.o" ), cleanOutput );
    AStackString<> reconstructed;
    reconstructed.Format( "%sapp_main.c%s", path.Get(), ext.Get() );
    TEST_ASSERT( PathUtils::ArePathsEqual( reconstructed, cleanOutput ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, NormalizesFASTBuildPathsPreservesDTLTOInput )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "tools\\subdir/../clang.exe" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "objects\\subdir/../app_main.c.obj" );
    job.m_Outputs.EmplaceBack( "app_main.c.1.native.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    // Check that path is normalized
    AStackString<> expectedCompiler;
    NodeGraph::CleanPath( data.m_CommonArgs[ 0 ], expectedCompiler );
    Node * compilerNode = nodeGraph.FindNode( AStackString<>( "Compiler-DTLTO" ) );
    TEST_ASSERT( compilerNode );
    AStackString<> compiler;
    TEST_ASSERT( compilerNode->GetReflectionInfoV()->GetProperty( compilerNode, "Executable", &compiler ) );
    TEST_ASSERT( compiler == expectedCompiler );

    // Check that input path is normalized
    Node * jobNode = FindObjectList( nodeGraph, "app_main.c.1.native.o" );
    TEST_ASSERT( jobNode );
    StackArray<AString> inputs;
    TEST_ASSERT( jobNode->GetReflectionInfoV()->GetProperty( jobNode, "CompilerInputFiles", &inputs ) );
    TEST_ASSERT( inputs.GetSize() == 1 );
    AStackString<> expectedInput;
    NodeGraph::CleanPath( job.m_Args[ 0 ], expectedInput );
    TEST_ASSERT( inputs[ 0 ] == expectedInput );

    // Check that Clang receives the original path
    AStackString<> options;
    TEST_ASSERT( jobNode->GetReflectionInfoV()->GetProperty( jobNode, "CompilerOptions", &options ) );
    TEST_ASSERT( options == "-D_FASTBUILD_DTLTO_INPUT=\"%1\" \"objects\\subdir/../app_main.c.obj\" -o \"%2\"" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, RejectsOutputNotMatchingInput )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "app_main.c.obj" );
    job.m_Outputs.EmplaceBack( "unrelated.o" ); // does not begin with input base "app_main.c"

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "does not begin with input base" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, RejectsJobWithoutOutputs )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "app_main.c.obj" ); // input present, but no outputs

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "job has no outputs" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, RejectsJobWithoutInput )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Outputs.EmplaceBack( "app_main.c.1.native.o" ); // output present, but no args

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) == nullptr ); //
    TEST_ASSERT( GetRecordedOutput().Find( "job has no input bitcode path" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, DropsExplicitOutputArgs )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    data.m_CommonArgs.EmplaceBack( "-common" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "input.bc" );
    job.m_Args.EmplaceBack( "-keepme" );
    job.m_Args.EmplaceBack( "-o" );
    job.m_Args.EmplaceBack( "DROP_SPACE" ); // dropped together with "-o"
    job.m_Args.EmplaceBack( "-o=DROP_EQ" ); // dropped
    job.m_Outputs.EmplaceBack( "input.1.native.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    Node * jobNode = FindObjectList( nodeGraph, "input.1.native.o" );
    TEST_ASSERT( jobNode );

    AStackString<> options;
    TEST_ASSERT( jobNode->GetReflectionInfoV()->GetProperty( jobNode, "CompilerOptions", &options ) );
    TEST_ASSERT( options == "-common -keepme -D_FASTBUILD_DTLTO_INPUT=\"%1\" \"input.bc\" -o \"%2\"" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, ReusesExistingCompilerNode )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data1;
    data1.m_CommonArgs.EmplaceBack( "clang.exe" );
    DTLTOData::Job & job1 = data1.m_Jobs.EmplaceBack();
    job1.m_Args.EmplaceBack( "app_main.c.obj" );
    job1.m_Outputs.EmplaceBack( "app_main.c.1.native.o" );

    DTLTOData data2;
    data2.m_CommonArgs.EmplaceBack( "clang.exe" ); // same compiler -> node reused
    DTLTOData::Job & job2 = data2.m_Jobs.EmplaceBack();
    job2.m_Args.EmplaceBack( "math.c.obj" );
    job2.m_Outputs.EmplaceBack( "math.c.2.native.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data1, AStackString<>( "dtlto-all-1" ) ) );
    TEST_ASSERT( builder.BuildGraph( data2, AStackString<>( "dtlto-all-2" ) ) ); // reuse, no assert

    Node * compiler = nodeGraph.FindNode( AStackString<>( "Compiler-DTLTO" ) );
    TEST_ASSERT( compiler );
    TEST_ASSERT( compiler->GetType() == Node::COMPILER_NODE );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, CacheKeyCompilerOptions )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    data.m_CommonArgs.EmplaceBack( "-O2" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "a.obj" );
    job.m_Args.EmplaceBack( "-fthinlto-index=a.thinlto.bc" );
    job.m_Args.EmplaceBack( "-jobflag" );
    job.m_Args.EmplaceBack( "-o" );
    job.m_Args.EmplaceBack( "a.o" );
    job.m_Outputs.EmplaceBack( "a.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    Node * jobNode = FindObjectList( nodeGraph, "a.o" );
    TEST_ASSERT( jobNode );

    AStackString<> cacheKeyOptions;
    TEST_ASSERT( jobNode->GetReflectionInfoV()->GetProperty( jobNode, "CacheKeyCompilerOptions", &cacheKeyOptions ) );
    TEST_ASSERT( cacheKeyOptions == "-O2 -jobflag a.obj" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, CacheKeyInputFiles )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" );
    data.m_CommonInputs.EmplaceBack( "extra.bc" );
    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "a.obj" );
    job.m_Outputs.EmplaceBack( "a.o" );
    job.m_Inputs.EmplaceBack( "a.obj" );
    job.m_Inputs.EmplaceBack( "a.thinlto.bc" );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    Node * jobNode = FindObjectList( nodeGraph, "a.o" );
    TEST_ASSERT( jobNode );
    const ReflectionInfo * ri = jobNode->GetReflectionInfoV();

    // Check compiler input files
    StackArray<AString> compilerInputs;
    TEST_ASSERT( ri->GetProperty( jobNode, "CompilerInputFiles", &compilerInputs ) );
    TEST_ASSERT( compilerInputs.GetSize() == 1 );
    AStackString<> expectedPrimary;
    NodeGraph::CleanPath( AStackString<>( "a.obj" ), expectedPrimary );
    TEST_ASSERT( compilerInputs[ 0 ] == expectedPrimary );

    // Check cache key input files
    StackArray<AString> cacheInputs;
    TEST_ASSERT( ri->GetProperty( jobNode, "CacheKeyInputFiles", &cacheInputs ) );
    TEST_ASSERT( cacheInputs.GetSize() == 2 );
    AStackString<> expectedIndex;
    AStackString<> expectedCommon;
    NodeGraph::CleanPath( AStackString<>( "a.thinlto.bc" ), expectedIndex );
    NodeGraph::CleanPath( AStackString<>( "extra.bc" ), expectedCommon );
    TEST_ASSERT( cacheInputs[ 0 ] == expectedIndex );
    TEST_ASSERT( cacheInputs[ 1 ] == expectedCommon );
}

//------------------------------------------------------------------------------
