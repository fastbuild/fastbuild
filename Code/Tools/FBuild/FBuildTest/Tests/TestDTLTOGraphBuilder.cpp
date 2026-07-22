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
    TEST_ASSERT( options == "-jobflag %1 -o \"%2\"" );
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
    TEST_ASSERT( options == "-O2 -fthinlto-index=a.thinlto.bc %1 -o \"%2\"" );
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
    job.m_Args.EmplaceBack( "input.bc" ); // %1, dropped from options
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
    TEST_ASSERT( options == "-common -keepme %1 -o \"%2\"" );
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
