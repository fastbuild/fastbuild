// DTLTOGraphBuilder
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "DTLTOGraphBuilder.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/ReflectionInfo.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
DTLTOGraphBuilder::DTLTOGraphBuilder( NodeGraph & nodeGraph )
    : m_NodeGraph( nodeGraph )
{
}

// BuildGraph
//------------------------------------------------------------------------------
Node * DTLTOGraphBuilder::BuildGraph( const DTLTOData & data, const AString & aliasName )
{
    if ( data.m_CommonArgs.IsEmpty() ) // m_CommonArgs[0] is the compiler
    {
        FLOG_ERROR( "DTLTO: no compiler specified" );
        return nullptr;
    }
    if ( data.m_Jobs.IsEmpty() )
    {
        FLOG_ERROR( "DTLTO: no jobs to build" );
        return nullptr;
    }
    if ( m_NodeGraph.FindNode( aliasName ) )
    {
        FLOG_ERROR( "DTLTO: target '%s' already exists", aliasName.Get() );
        return nullptr;
    }

    // Create the compiler node
    CompilerNode * compiler = CreateCompilerNode( data.m_CommonArgs[ 0 ] );
    if ( compiler == nullptr )
    {
        return nullptr;
    }

    // Create the object list nodes for each job
    StackArray<Node *> jobNodes;
    for ( const DTLTOData::Job & job : data.m_Jobs )
    {
        Node * objectList = CreateObjectListForJob( data, job, compiler );
        if ( objectList == nullptr )
        {
            return nullptr;
        }
        jobNodes.Append( objectList );
    }

    // Group all jobs under a single root to build
    AliasNode * root = m_NodeGraph.CreateNode<AliasNode>( aliasName );

    const ReflectedProperty * targetsProp =
        root->GetReflectionInfoV()->GetReflectedProperty( AStackString( "Targets" ) );
    ASSERT( targetsProp );
    targetsProp->GetPtrToArray<Node *>( root )->Append( jobNodes );

    if ( !root->Initialize( m_NodeGraph, nullptr, Function::Find( AStackString( "Alias" ) ) ) )
    {
        return nullptr;
    }

    return root;
}

// CreateCompilerNode
//------------------------------------------------------------------------------
CompilerNode * DTLTOGraphBuilder::CreateCompilerNode( const AString & compilerExe )
{
    const AStackString compilerName( "Compiler-DTLTO" );
    if ( Node * existing = m_NodeGraph.FindNode( compilerName ) )
    {
        return existing->CastTo<CompilerNode>();
    }

    AStackString<> cleanCompilerExe;
    NodeGraph::CleanPath( compilerExe, cleanCompilerExe );

    CompilerNode * compiler = m_NodeGraph.CreateNode<CompilerNode>( compilerName );
    const ReflectionInfo * ri = compiler->GetReflectionInfoV();
    VERIFY( ri->SetProperty( compiler, "Executable", cleanCompilerExe ) );
    VERIFY( ri->SetProperty( compiler, "CompilerFamily", AStackString( "custom" ) ) );
    VERIFY( ri->SetProperty( compiler, "SimpleDistributionMode", true ) );
    VERIFY( ri->SetProperty( compiler, "AllowDistribution", true ) );

    if ( !compiler->Initialize( m_NodeGraph, nullptr, Function::Find( AStackString( "Compiler" ) ) ) )
    {
        return nullptr;
    }
    return compiler;
}

// CreateObjectListForJob
//------------------------------------------------------------------------------
Node * DTLTOGraphBuilder::CreateObjectListForJob( const DTLTOData & data,
                                                  const DTLTOData::Job & job,
                                                  CompilerNode * compiler )
{
    if ( job.m_Outputs.IsEmpty() )
    {
        FLOG_ERROR( "DTLTO: job has no outputs" );
        return nullptr;
    }

    if ( job.m_Args.IsEmpty() )
    {
        FLOG_ERROR( "DTLTO: job has no input bitcode path" );
        return nullptr;
    }
    AStackString<> compilerOptions;
    AStackString<> cacheKeyCompilerOptions;
    BuildCompilerOptions( data.m_CommonArgs, job.m_Args, compilerOptions, cacheKeyCompilerOptions );

    AStackString<> cleanOutput;
    NodeGraph::CleanPath( job.m_Outputs[ 0 ], cleanOutput );

    // Construct unique list name for given job
    AStackString<> listName;
    listName.Format( "DTLTO-List:%s", cleanOutput.Get() );

    ObjectListNode * result = m_NodeGraph.CreateNode<ObjectListNode>( listName );

    // Get the base name of the input file
    AStackString<> cleanInput;
    NodeGraph::CleanPath( job.m_Args[ 0 ], cleanInput );  // the compiled module is the first positional job arg
    AStackString<> inputBase;
    result->GetObjectFileName( cleanInput, AString::GetEmpty(), inputBase );
    inputBase.SetLength( inputBase.GetLength() - (uint32_t)AString::StrLen( result->GetObjExtension() ) );

    // Get the directory and file name of the output file
    const char * lastSlash = cleanOutput.FindLast( NATIVE_SLASH );
    const AStackString<> outputDir( cleanOutput.Get(), lastSlash + 1 );
    const AStackString<> outputFileName( lastSlash + 1, cleanOutput.GetEnd() );

    // Get proper "CompilerOutputExtension" so the ObjectListNode emits the required output file name.
    // e.g. input = "app_main.c.obj", required output file name = "app_main.c.1.25872.native.o" =>
    //   node output = "app_main.c" (input base) + ".1.25872.native.o" (CompilerOutputExtension)
    if ( !outputFileName.BeginsWith( inputBase ) )
    {
        FLOG_ERROR( "DTLTO: output '%s' does not begin with input base '%s'",
                    outputFileName.Get(),
                    inputBase.Get() );
        return nullptr;
    }
    const AStackString<> outputExtension( outputFileName.Get() + inputBase.GetLength(), outputFileName.GetEnd() );

    // Prepare the input files
    StackArray<AString> inputFiles;
    inputFiles.EmplaceBack( cleanInput );

    // Extra files for the cache key (ThinLTO index / imports)
    StackArray<AString> cacheKeyInputFiles;
    for ( const AString & input : job.m_Inputs )
    {
        AStackString<> cleanPath;
        NodeGraph::CleanPath( input, cleanPath );
        if ( PathUtils::ArePathsEqual( cleanPath, cleanInput ) == false ) //
        {
            cacheKeyInputFiles.EmplaceBack( cleanPath );
        }
    }
    for ( const AString & input : data.m_CommonInputs )
    {
        AStackString<> cleanPath;
        NodeGraph::CleanPath( input, cleanPath );
        cacheKeyInputFiles.EmplaceBack( cleanPath );
    }

    // Set the properties of the ObjectListNode
    const ReflectionInfo * ri = result->GetReflectionInfoV();
    VERIFY( ri->SetProperty( result, "Compiler", compiler->GetName() ) );
    VERIFY( ri->SetProperty( result, "CompilerOptions", compilerOptions ) );
    VERIFY( ri->SetProperty( result, "CompilerOutputPath", outputDir ) );
    VERIFY( ri->SetProperty( result, "CompilerOutputExtension", outputExtension ) );
    VERIFY( ri->SetProperty( result, "AllowDistribution", true ) );
    VERIFY( ri->SetProperty( result, "CompilerInputFiles", inputFiles ) );
    VERIFY( ri->SetProperty( result, "CacheKeyInputFiles", cacheKeyInputFiles ) );
    VERIFY( ri->SetProperty( result, "CacheKeyCompilerOptions", cacheKeyCompilerOptions ) );

    if ( !result->Initialize( m_NodeGraph, nullptr, Function::Find( AStackString( "ObjectList" ) ) ) )
    {
        return nullptr;
    }

    return result;
}

// BuildCompilerOptions
//------------------------------------------------------------------------------
/*static*/ void DTLTOGraphBuilder::BuildCompilerOptions( const Array<AString> & commonArgs,
                                                         const Array<AString> & jobArgs,
                                                         AString & outOptions,
                                                         AString & outOptionsForCacheKey )
{
    outOptions.Clear();
    outOptionsForCacheKey.Clear();

    // Skip commonArgs[0] (compiler exe, which lives on the CompilerNode)
    for ( size_t i = 1; i < commonArgs.GetSize(); ++i )
    {
        outOptions += commonArgs[ i ];
        outOptions += ' ';
        outOptionsForCacheKey += commonArgs[ i ];
        outOptionsForCacheKey += ' ';
    }

    // Skip jobArgs[0] (input, appended below) and any -o (output is %2)
    for ( size_t i = 1; i < jobArgs.GetSize(); ++i )
    {
        const AString & a = jobArgs[ i ];
        if ( a == "-o" )
        {
            ++i; // drop "-o <path>"
            continue;
        }
        if ( a.BeginsWith( "-o=" ) )
        {
            continue;
        }
        outOptions += a;
        outOptions += ' ';

        // The index file name embeds the linker pid - it's pointless to hash it
        // Its contents is hashed via CacheKeyInputFiles
        if ( a.BeginsWith( "-fthinlto-index=" ) == false )
        {
            outOptionsForCacheKey += a;
            outOptionsForCacheKey += ' ';
        }
    }

    // ObjectList requires %1, but expands it to an absolute path
    // ThinLTO identifies modules by the original path from the JSON
    // So use %1 only in inert define and pass the original input path to clang
    outOptions += "-D_FASTBUILD_DTLTO_INPUT=\"%1\"";
    outOptions.AppendFormat( " \"%s\"", jobArgs[ 0 ].Get() );
    outOptions += " -o \"%2\"";

    outOptionsForCacheKey += jobArgs[ 0 ];
}

//------------------------------------------------------------------------------
