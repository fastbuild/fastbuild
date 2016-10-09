// LibraryNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "LibraryNode.h"
#include "DirectoryListNode.h"
#include "UnityNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
LibraryNode::LibraryNode( const AString & libraryName,
                          const Dependencies & inputNodes,
                          CompilerNode * compiler,
                          const AString & compilerArgs,
                          const AString & compilerArgsDeoptimized,
                          const AString & compilerOutputPath,
                          const AString & librarian,
                          const AString & librarianArgs,
                          uint32_t flags,
                          ObjectNode * precompiledHeader,
                          const Dependencies & compilerForceUsing,
                          const Dependencies & preBuildDependencies,
                          const Dependencies & additionalInputs,
                          bool deoptimizeWritableFiles,
                          bool deoptimizeWritableFilesWithToken,
                          bool allowDistribution,
                          bool allowCaching,
                          CompilerNode * preprocessor,
                          const AString & preprocessorArgs,
                          const AString & baseDirectory )
: ObjectListNode( libraryName,
                  inputNodes,
                  compiler,
                  compilerArgs,
                  compilerArgsDeoptimized,
                  compilerOutputPath,
                  precompiledHeader,
                  compilerForceUsing,
                  preBuildDependencies,
                  deoptimizeWritableFiles,
                  deoptimizeWritableFilesWithToken,
                  allowDistribution,
                  allowCaching,
                  preprocessor,
                  preprocessorArgs,
                  baseDirectory )
, m_AdditionalInputs( additionalInputs )
{
    m_Type = LIBRARY_NODE;
    m_LastBuildTimeMs = 10000; // TODO:C Reduce this when dynamic deps are saved

    m_LibrarianPath = librarian; // TODO:C This should be a node
    m_LibrarianArgs = librarianArgs;
    m_Flags = flags;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
LibraryNode::~LibraryNode() = default;

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool LibraryNode::IsAFile() const
{
    return true;
}

// GatherDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool LibraryNode::GatherDynamicDependencies( NodeGraph & nodeGraph, bool forceClean )
{
    if ( ObjectListNode::GatherDynamicDependencies( nodeGraph, forceClean ) == false )
    {
        return false; // GatherDynamicDependencies will have emited an error
    }

    // additional libs/objects
    m_DynamicDependencies.Append( m_AdditionalInputs );
    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult LibraryNode::DoBuild( Job * UNUSED( job ) )
{
    // delete library before creation (so ar.exe will not merge old symbols)
    if ( FileIO::FileExists( GetName().Get() ) )
    {
        FileIO::FileDelete( GetName().Get() );
    }

    // Format compiler args string
    Args fullArgs;
    if ( !BuildArgs( fullArgs ) )
    {
        return NODE_RESULT_FAILED; // BuildArgs will have emitted an error
    }

    // use the exe launch dir as the working dir
    const char * workingDir = nullptr;

    const char * environment = FBuild::Get().GetEnvironmentString();

    EmitCompilationMessage( fullArgs );

    // spawn the process
    Process p;
    bool spawnOK = p.Spawn( m_LibrarianPath.Get(),
                            fullArgs.GetFinalArgs().Get(),
                            workingDir,
                            environment );

    if ( !spawnOK )
    {
        FLOG_ERROR( "Failed to spawn process for Library creation for '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // capture all of the stdout and stderr
    AutoPtr< char > memOut;
    AutoPtr< char > memErr;
    uint32_t memOutSize = 0;
    uint32_t memErrSize = 0;
    p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

    ASSERT( !p.IsRunning() );
    // Get result
    int result = p.WaitForExit();

    if ( result != 0 )
    {
        if ( memOut.Get() )
        {
            m_BuildOutputMessages.Append( memOut.Get(), memOutSize );
            FLOG_ERROR_DIRECT( memOut.Get() );
        }

        if ( memErr.Get() )
        {
            m_BuildOutputMessages.Append( memErr.Get(), memErrSize );
            FLOG_ERROR_DIRECT( memErr.Get() );
        }
    }

    // did the executable fail?
    if ( result != 0 )
    {
        FLOG_ERROR( "Failed to build Library (error %i) '%s'", result, GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // record time stamp for next time
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    ASSERT( m_Stamp );

    return NODE_RESULT_OK;
}

// BuildArgs
//------------------------------------------------------------------------------
bool LibraryNode::BuildArgs( Args & fullArgs ) const
{
    Array< AString > tokens( 1024, true );
    m_LibrarianArgs.Tokenize( tokens );

    const AString * const end = tokens.End();
    for ( const AString * it = tokens.Begin(); it!=end; ++it )
    {
        const AString & token = *it;
        if ( token.EndsWith( "%1" ) )
        {
            // handle /Option:%1 -> /Option:A /Option:B /Option:C
            AStackString<> pre;
            if ( token.GetLength() > 2 )
            {
                pre.Assign( token.Get(), token.GetEnd() - 2 );
            }

            // concatenate files, unquoted
            GetInputFiles( fullArgs, pre, AString::GetEmpty() );
        }
        else if ( token.EndsWith( "\"%1\"" ) )
        {
            // handle /Option:"%1" -> /Option:"A" /Option:"B" /Option:"C"
            AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote
            AStackString<> post( "\"" );

            // concatenate files, quoted
            GetInputFiles( fullArgs, pre, post );
        }
        else if ( token.EndsWith( "%2" ) )
        {
            // handle /Option:%2 -> /Option:A
            if ( token.GetLength() > 2 )
            {
                fullArgs += AStackString<>( token.Get(), token.GetEnd() - 2 );
            }
            fullArgs += m_Name;
        }
        else if ( token.EndsWith( "\"%2\"" ) )
        {
            // handle /Option:"%2" -> /Option:"A"
            AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote
            fullArgs += pre;
            fullArgs += m_Name;
            fullArgs += '"'; // post
        }
        else
        {
            fullArgs += token;
        }

        fullArgs.AddDelimiter();
    }

    // orbis-ar.exe requires escaped slashes inside response file
    if ( GetFlag( LIB_FLAG_ORBIS_AR ) )
    {
        fullArgs.SetEscapeSlashesInResponseFile();
    }

    // Handle all the special needs of args
    if ( fullArgs.Finalize( m_LibrarianPath, GetName(), CanUseResponseFile() ) == false )
    {
        return false; // Finalize will have emitted an error
    }

    return true;
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t LibraryNode::DetermineFlags( const AString & librarianName )
{
    uint32_t flags = 0;
    if ( librarianName.EndsWithI("lib.exe") ||
        librarianName.EndsWithI("lib") ||
        librarianName.EndsWithI("link.exe") ||
        librarianName.EndsWithI("link"))
    {
        flags |= LIB_FLAG_LIB;
    }
    else if ( librarianName.EndsWithI("ar.exe") ||
         librarianName.EndsWithI("ar") )
    {
        if ( librarianName.FindI( "orbis-ar" ) )
        {
            flags |= LIB_FLAG_ORBIS_AR;
        }
        else
        {
            flags |= LIB_FLAG_AR;
        }
    }
    else if ( librarianName.EndsWithI( "\\ax.exe" ) ||
              librarianName.EndsWithI( "\\ax" ) )
    {
        flags |= LIB_FLAG_GREENHILLS_AX;
    }
    return flags;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void LibraryNode::EmitCompilationMessage( const Args & fullArgs ) const
{
    AStackString<> output;
    output += "Lib: ";
    output += GetName();
    output += '\n';
    if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
    {
        output += m_LibrarianPath;
        output += ' ';
        output += fullArgs.GetRawArgs();
        output += '\n';
    }
    FLOG_BUILD_DIRECT( output.Get() );
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * LibraryNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>,  name );
    NODE_LOAD_NODE( CompilerNode,   compilerNode );
    NODE_LOAD( AStackString<>,  compilerArgs );
    NODE_LOAD( AStackString<>,  compilerArgsDeoptimized );
    NODE_LOAD( AStackString<>,  compilerOutputPath );
    NODE_LOAD_DEPS( 16,         staticDeps );
    NODE_LOAD_NODE( Node,       precompiledHeader );
    NODE_LOAD( AStackString<>,  objExtensionOverride );
    NODE_LOAD( AStackString<>,  compilerOutputPrefix );
    NODE_LOAD_DEPS( 0,          compilerForceUsing );
    NODE_LOAD_DEPS( 0,          preBuildDependencies );
    NODE_LOAD( bool,            deoptimizeWritableFiles );
    NODE_LOAD( bool,            deoptimizeWritableFilesWithToken );
    NODE_LOAD( bool,            allowDistribution );
    NODE_LOAD( bool,            allowCaching );
    NODE_LOAD_NODE( CompilerNode, preprocessorNode );
    NODE_LOAD( AStackString<>,  preprocessorArgs );
    NODE_LOAD( AStackString<>,  baseDirectory );
    NODE_LOAD( AStackString<>,  extraPDBPath );
    NODE_LOAD( AStackString<>,  extraASMPath );

    NODE_LOAD( AStackString<>,  librarianPath );
    NODE_LOAD( AStackString<>,  librarianArgs );
    NODE_LOAD( uint32_t,        flags );
    NODE_LOAD_DEPS( 0,          additionalInputs );

    LibraryNode * n = nodeGraph.CreateLibraryNode( name,
                                 staticDeps,
                                 compilerNode,
                                 compilerArgs,
                                 compilerArgsDeoptimized,
                                 compilerOutputPath,
                                 librarianPath,
                                 librarianArgs,
                                 flags,
                                 precompiledHeader ? precompiledHeader->CastTo< ObjectNode >() : nullptr,
                                 compilerForceUsing,
                                 preBuildDependencies,
                                 additionalInputs,
                                 deoptimizeWritableFiles,
                                 deoptimizeWritableFilesWithToken,
                                 allowDistribution,
                                 allowCaching,
                                 preprocessorNode,
                                 preprocessorArgs,
                                 baseDirectory );
    n->m_ObjExtensionOverride = objExtensionOverride;
    n->m_CompilerOutputPrefix = compilerOutputPrefix;
    n->m_ExtraPDBPath = extraPDBPath;
    n->m_ExtraASMPath = extraASMPath;

    // TODO:B Need to save the dynamic deps, for better progress estimates
    // but we can't right now because we rely on the nodes we depend on
    // being saved before us which isn't the case for dynamic deps.
    //if ( Node::LoadDepArray( fileStream, n->m_DynamicDependencies ) == false )
    //{
    //  FDELETE n;
    //  return nullptr;
    //}
    return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void LibraryNode::Save( IOStream & stream ) const
{
    ObjectListNode::Save( stream );

    NODE_SAVE( m_LibrarianPath );
    NODE_SAVE( m_LibrarianArgs );
    NODE_SAVE( m_Flags );
    NODE_SAVE_DEPS( m_AdditionalInputs );
}

// CanUseResponseFile
//------------------------------------------------------------------------------
bool LibraryNode::CanUseResponseFile() const
{
    #if defined( __WINDOWS__ )
        // Generally only windows applications support response files (to overcome Windows command line limits)
        return ( GetFlag( LIB_FLAG_LIB ) || GetFlag( LIB_FLAG_AR ) || GetFlag( LIB_FLAG_ORBIS_AR ) || GetFlag( LIB_FLAG_GREENHILLS_AX ) );
    #else
        return false;
    #endif
}

//------------------------------------------------------------------------------
