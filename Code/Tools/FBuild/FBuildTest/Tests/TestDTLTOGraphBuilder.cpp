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
#include "Core/Reflection/ReflectionInfo.h"
#include "Core/Strings/AStackString.h"

// TestDTLTOGraphBuilder
//------------------------------------------------------------------------------
TEST_GROUP( TestDTLTOGraphBuilder, FBuildTest )
{
};

// MakeDTLTOData
//------------------------------------------------------------------------------
static void MakeDTLTOData( DTLTOData & data )
{
    data.m_CommonArgs.EmplaceBack( "clang.exe" ); // args[0] is the compiler
    data.m_CommonArgs.EmplaceBack( "-c" );
    data.m_CommonArgs.EmplaceBack( "-O2" );

    {
        DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
        job.m_Args.EmplaceBack( "app_main.c.obj" );
        job.m_Args.EmplaceBack( "-fthinlto-index=app_main.c.1.native.o.thinlto.bc" );
        job.m_Outputs.EmplaceBack( "app_main.c.1.native.o" );
    }
    {
        DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
        job.m_Outputs.EmplaceBack( "math.c.2.native.o" );
    }
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, BuildSynthesizesGraph )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    MakeDTLTOData( data );

    DTLTOGraphBuilder builder( nodeGraph );
    const Node * alias = builder.BuildGraph( data, AStackString<>( "dtlto-all" ) );

    TEST_ASSERT( alias );
    TEST_ASSERT( alias->GetType() == Node::ALIAS_NODE );
    TEST_ASSERT( alias->GetStaticDependencies().GetSize() == 2 ); // 2 jobs

    AStackString<> expectedName;
    NodeGraph::CleanPath( AStackString<>( "app_main.c.1.native.o" ), expectedName );
    Node * jobNode = nodeGraph.FindNode( expectedName );
    TEST_ASSERT( jobNode );
    TEST_ASSERT( jobNode->GetType() == Node::EXEC_NODE );

    AStackString<> args;
    TEST_ASSERT( jobNode->GetReflectionInfoV()->GetProperty( jobNode, "ExecArguments", &args ) );
    TEST_ASSERT( args.Find( "-c" ) ); // common args
    TEST_ASSERT( args.Find( "-O2" ) ); // common args
    TEST_ASSERT( args.Find( "-fthinlto-index=app_main.c.1.native.o.thinlto.bc" ) ); // job args
    TEST_ASSERT( args.Find( "app_main.c.obj" ) ); // job args
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
    data.m_CommonArgs.EmplaceBack( "clang.exe" ); // args[0] is the compiler

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
    MakeDTLTOData( data );

    DTLTOGraphBuilder builder( nodeGraph );
    TEST_ASSERT( builder.BuildGraph( data, AStackString<>( "dtlto-all" ) ) );

    // second build with the same target name must be rejected
    const Node * alias = builder.BuildGraph( data, AStackString<>( "dtlto-all" ) );
    TEST_ASSERT( alias == nullptr );
    TEST_ASSERT( GetRecordedOutput().Find( "already exists" ) );
}

//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, QuotesArguments )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    DTLTOData data;
    data.m_CommonArgs.EmplaceBack( "clang.exe" ); // args[0] is the compiler

    DTLTOData::Job & job = data.m_Jobs.EmplaceBack();
    job.m_Args.EmplaceBack( "C:\\Program Files\\in.o" ); // space       -> wrapped in quotes
    job.m_Args.EmplaceBack( "-DMSG=\"hi\"" );            // has a quote -> wrapped and escaped
    job.m_Args.EmplaceBack();                            // empty       -> ""
    job.m_Args.EmplaceBack( "plain" );                   // no specials -> unchanged
    job.m_Outputs.EmplaceBack( "out.o" );

    DTLTOGraphBuilder builder( nodeGraph );
    const Node * alias = builder.BuildGraph( data, AStackString<>( "dtlto-quote" ) );
    TEST_ASSERT( alias );

    AStackString<> nodeName;
    NodeGraph::CleanPath( AStackString<>( "out.o" ), nodeName );
    Node * jobNode = nodeGraph.FindNode( nodeName );
    TEST_ASSERT( jobNode );

    AStackString<> args;
    TEST_ASSERT( jobNode->GetReflectionInfoV()->GetProperty( jobNode, "ExecArguments", &args ) );
    TEST_ASSERT( args.Find( "\"C:\\Program Files\\in.o\"" ) ); // space -> quoted
    TEST_ASSERT( args.Find( "\"-DMSG=\\\"hi\\\"\"" ) );        // quote -> escaped + quoted
    TEST_ASSERT( args.Find( " \"\" " ) );                      // empty -> ""
    TEST_ASSERT( args.Find( "plain" ) );                       // unchanged
}

// Real DTLTO JSON -> parser -> builder end-to-end
//------------------------------------------------------------------------------
TEST_CASE( TestDTLTOGraphBuilder, BuildFromParsedJson )
{
    FBuild fBuild;
    NodeGraph nodeGraph;

    AStackString<> json( R"({
        "common": {
            "linker_output": "app.exe",
            "args": [ "clang.exe", "-c", "--target=x86_64-pc-windows-msvc", "-O2" ],
            "inputs": []
        },
        "jobs": [
            {
                "args": [ "app_main.c.obj", "-fthinlto-index=app_main.c.1.native.o.thinlto.bc", "-o", "app_main.c.1.native.o" ],
                "inputs": [ "app_main.c.obj", "app_main.c.1.native.o.thinlto.bc" ],
                "outputs": [ "app_main.c.1.native.o" ]
            },
            {
                "args": [ "math.c.obj", "-fthinlto-index=math.c.2.native.o.thinlto.bc", "-o", "math.c.2.native.o" ],
                "inputs": [ "math.c.obj", "math.c.2.native.o.thinlto.bc" ],
                "outputs": [ "math.c.2.native.o" ]
            }
        ]
    })" );

    DTLTOJsonParser parser( json );
    TEST_ASSERT( parser.Parse() );

    DTLTOGraphBuilder builder( nodeGraph );
    const Node * alias = builder.BuildGraph( parser.GetData(), AStackString<>( "dtlto-all" ) );
    TEST_ASSERT( alias );
    TEST_ASSERT( alias->GetType() == Node::ALIAS_NODE );
    TEST_ASSERT( alias->GetStaticDependencies().GetSize() == 2 ); // 2 jobs

    AStackString<> nodeName;
    NodeGraph::CleanPath( AStackString<>( "app_main.c.1.native.o" ), nodeName );
    Node * jobNode = nodeGraph.FindNode( nodeName );
    TEST_ASSERT( jobNode );
    TEST_ASSERT( jobNode->GetType() == Node::EXEC_NODE );

    const ReflectionInfo * ri = jobNode->GetReflectionInfoV();

    AStackString<> exe;
    TEST_ASSERT( ri->GetProperty( jobNode, "ExecExecutable", &exe ) );
    TEST_ASSERT( exe.EndsWith( "clang.exe" ) ); // compiler = common.args[0]

    AStackString<> args;
    TEST_ASSERT( ri->GetProperty( jobNode, "ExecArguments", &args ) );
    TEST_ASSERT( args.Find( "-c" ) );                                                   // common flag
    TEST_ASSERT( args.Find( "-O2" ) );                                                  // common flag
    TEST_ASSERT( args.Find( "-fthinlto-index=app_main.c.1.native.o.thinlto.bc" ) );     // job arg
    TEST_ASSERT( args.Find( "app_main.c.obj" ) );                                       // job arg
    TEST_ASSERT( args.Find( "clang.exe" ) == nullptr );                                 // compiler is not duplicated into args
}

//------------------------------------------------------------------------------
