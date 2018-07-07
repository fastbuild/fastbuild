// LibraryNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "LibraryNode.h"
#include "DirectoryListNode.h"
#include "UnityNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LinkerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/Helpers/ResponseFile.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( LibraryNode, ObjectListNode, MetaName( "LibrarianOutput" ) + MetaFile() )
    REFLECT( m_Librarian,                       "Librarian",                    MetaFile() )
    REFLECT( m_LibrarianOptions,                "LibrarianOptions",             MetaNone() )
    REFLECT( m_LibrarianOutput,                 "LibrarianOutput",              MetaFile() )
    REFLECT_ARRAY( m_LibrarianAdditionalInputs, "LibrarianAdditionalInputs",    MetaOptional() + MetaFile() + MetaAllowNonFile( Node::OBJECT_LIST_NODE ) )

    REFLECT( m_NumLibrarianAdditionalInputs,    "NumLibrarianAdditionalInputs", MetaHidden() )
    REFLECT( m_LibrarianFlags,                  "LibrarianFlags",               MetaHidden() )
REFLECT_END( LibraryNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
LibraryNode::LibraryNode()
: ObjectListNode()
{
    m_Type = LIBRARY_NODE;
    m_LastBuildTimeMs = 10000; // TODO:C Reduce this when dynamic deps are saved
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool LibraryNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .Librarian
    Dependencies librarian;
    if ( !Function::GetFileNode( nodeGraph, iter, function, m_Librarian, "Librarian", librarian ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    ASSERT( librarian.GetSize() == 1 ); // Should only be possible to be one
    m_StaticDependencies.Append( librarian );
    m_ObjectListInputStartIndex += 1; // Ensure librarian is not treated as an input

    // .LibrarianOptions
    {
        if ( m_LibrarianOptions.Find( "%1" ) == nullptr )
        {
            Error::Error_1106_MissingRequiredToken( iter, function, ".LibrarianOptions", "%1" );
            return false;
        }
        if ( m_LibrarianOptions.Find( "%2" ) == nullptr )
        {
            Error::Error_1106_MissingRequiredToken( iter, function, ".LibrarianOptions", "%2" );
            return false;
        }
    }

    // Handle all the ObjectList common stuff
    if ( !ObjectListNode::Initialize( nodeGraph, iter, function ) )
    {
        return false;
    }

    // .LibrarianAdditionalInputs
    Dependencies librarianAdditionalInputs;
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".LibrarianAdditionalInputs", m_LibrarianAdditionalInputs, librarianAdditionalInputs ) )
    {
        return false;// GetNodeList will emit error
    }
    m_NumLibrarianAdditionalInputs = (uint32_t)librarianAdditionalInputs.GetSize();

    // Store dependencies
    m_StaticDependencies.SetCapacity( m_StaticDependencies.GetSize() + librarianAdditionalInputs.GetSize() );
    m_StaticDependencies.Append( librarianAdditionalInputs );
    // m_ObjectListInputEndIndex // NOTE: Deliberately not added to m_ObjectListInputEndIndex, since we don't want to try and compile these things

    m_LibrarianFlags = DetermineFlags( m_Librarian, m_LibrarianOptions );

    return true;
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

    // .LibrarianAdditionalInputs
    // (By simply pushing these into the DynamicDeps, the existing ObjectListNode logic will
    //  handle expanding them into the command line like everything else)
    const size_t startIndex = m_StaticDependencies.GetSize() - m_NumLibrarianAdditionalInputs;
    const size_t endIndex = m_StaticDependencies.GetSize();
    for ( size_t i=startIndex; i<endIndex; ++i )
    {
        m_DynamicDependencies.Append( Dependency( m_StaticDependencies[ i ].GetNode() ) );
    }
    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult LibraryNode::DoBuild( Job * job )
{
    // Delete previous file(s) if doing a clean build
    if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
    {
        if ( FileIO::FileExists( GetName().Get() ) )
        {
            FileIO::FileDelete( GetName().Get() );
        }
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
    Process p( FBuild::Get().GetAbortBuildPointer() );
    bool spawnOK = p.Spawn( GetLibrarian()->GetName().Get(),
                            fullArgs.GetFinalArgs().Get(),
                            workingDir,
                            environment );

    if ( !spawnOK )
    {
        if ( p.HasAborted() )
        {
            return NODE_RESULT_FAILED;
        }

        FLOG_ERROR( "Failed to spawn process for Library creation for '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    // capture all of the stdout and stderr
    AutoPtr< char > memOut;
    AutoPtr< char > memErr;
    uint32_t memOutSize = 0;
    uint32_t memErrSize = 0;
    p.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );

    // Get result
    int result = p.WaitForExit();
    if ( p.HasAborted() )
    {
        return NODE_RESULT_FAILED;
    }

    // did the executable fail?
    if ( result != 0 )
    {
        if ( memOut.Get() )
        {
            job->ErrorPreformatted( memOut.Get() );
        }

        if ( memErr.Get() )
        {
            job->ErrorPreformatted( memErr.Get() );
        }

        FLOG_ERROR( "Failed to build Library (error %i) '%s'", result, GetName().Get() );
        return NODE_RESULT_FAILED;
    }
    else
    {
        // If "warnings as errors" is enabled (/WX) we don't need to check
        // (since compilation will fail anyway, and the output will be shown)
        if ( GetFlag( LIB_FLAG_LIB ) && !GetFlag( LIB_FLAG_WARNINGS_AS_ERRORS_MSVC ) )
        {
            FileNode::HandleWarningsMSVC( job, GetName(), memOut.Get(), memOutSize );
        }
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
    m_LibrarianOptions.Tokenize( tokens );

    // When merging libs for non-MSVC toolchains, merge the source
    // objects instead of the libs
    const bool objectsInsteadOfLibs = ( m_LibrarianFlags & LIB_FLAG_LIB ) ? false : true;

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
            GetInputFiles( fullArgs, pre, AString::GetEmpty(), objectsInsteadOfLibs );
        }
        else if ( token.EndsWith( "\"%1\"" ) )
        {
            // handle /Option:"%1" -> /Option:"A" /Option:"B" /Option:"C"
            AStackString<> pre( token.Get(), token.GetEnd() - 3 ); // 3 instead of 4 to include quote
            AStackString<> post( "\"" );

            // concatenate files, quoted
            GetInputFiles( fullArgs, pre, post, objectsInsteadOfLibs );
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
    if ( GetFlag( LIB_FLAG_ORBIS_AR ) || GetFlag( LIB_FLAG_AR ) )
    {
        fullArgs.SetEscapeSlashesInResponseFile();
    }

    // Handle all the special needs of args
    if ( fullArgs.Finalize( GetLibrarian()->GetName(), GetName(), CanUseResponseFile() ) == false )
    {
        return false; // Finalize will have emitted an error
    }

    return true;
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t LibraryNode::DetermineFlags( const AString & librarianName, const AString & args )
{
    uint32_t flags = 0;
    if ( librarianName.EndsWithI("lib.exe") ||
        librarianName.EndsWithI("lib") ||
        librarianName.EndsWithI("link.exe") ||
        librarianName.EndsWithI("link"))
    {
        flags |= LIB_FLAG_LIB;

        // Parse args for some other flags
        Array< AString > tokens;
        args.Tokenize( tokens );

        const AString * const end = tokens.End();
        for ( const AString * it = tokens.Begin(); it != end; ++it )
        {
            const AString & token = *it;
            if ( LinkerNode::IsLinkerArg_MSVC( token, "WX" ) )
            {
                flags |= LIB_FLAG_WARNINGS_AS_ERRORS_MSVC;
                continue;
            }
        }
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
        output += m_Librarian;
        output += ' ';
        output += fullArgs.GetRawArgs();
        output += '\n';
    }
    FLOG_BUILD_DIRECT( output.Get() );
}

// GetLibrarian
//------------------------------------------------------------------------------
FileNode * LibraryNode::GetLibrarian() const
{
    // Librarian is always at index 0
    return m_StaticDependencies[ 0 ].GetNode()->CastTo< FileNode >();
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
