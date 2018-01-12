// LinkerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "LinkerNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Error.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Process.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( LinkerNode, Node, MetaName( "LinkerOutput" ) + MetaFile() )
    REFLECT( m_Linker,                          "Linker",                       MetaFile() )
    REFLECT( m_LinkerOptions,                   "LinkerOptions",                MetaNone() )
    REFLECT( m_LinkerType,                      "LinkerType",                   MetaOptional() )
    REFLECT_ARRAY( m_Libraries,                 "Libraries",                    MetaFile() + MetaAllowNonFile() )
    REFLECT_ARRAY( m_LinkerAssemblyResources,   "LinkerAssemblyResources",      MetaOptional() + MetaFile() )
    REFLECT( m_LinkerLinkObjects,               "LinkerLinkObjects",            MetaOptional() )
    REFLECT( m_LinkerStampExe,                  "LinkerStampExe",               MetaOptional() + MetaFile() )
    REFLECT( m_LinkerStampExeArgs,              "LinkerStampExeArgs",           MetaOptional() )
    REFLECT_ARRAY( m_PreBuildDependencyNames,   "PreBuildDependencies",         MetaOptional() + MetaFile() + MetaAllowNonFile() )

    // Internal State
    REFLECT( m_Flags,                           "Flags",                        MetaHidden() )
    REFLECT( m_AssemblyResourcesStartIndex,     "AssemblyResourcesStartIndex",  MetaHidden() )
    REFLECT( m_AssemblyResourcesNum,            "AssemblyResourcesNum",         MetaHidden() )
    REFLECT( m_ImportLibName,                   "ImportLibName",                MetaHidden() )
REFLECT_END( LinkerNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
LinkerNode::LinkerNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
    , m_LinkerType( "auto" )
{
    m_LastBuildTimeMs = 20000; // Assume link times are fairly long by default
}

// Initialize
//------------------------------------------------------------------------------
bool LinkerNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // Get linker exe
    Node * linkerExeNode = nullptr;
    if ( !function->GetFileNode( nodeGraph, iter, linkerExeNode, ".Linker" ) ) // TODO:B Use m_Linker property
    {
        return false; // GetFileNode will have emitted an error
    }

    m_Flags = DetermineFlags( m_LinkerType, m_Linker, m_LinkerOptions );

    // Check for Import Library override
    if ( ( m_Flags & LinkerNode::LINK_FLAG_MSVC ) != 0 )
    {
        GetImportLibName( m_LinkerOptions, m_ImportLibName );
    }

    // Check input/output args for Linker
    {
        bool hasInputToken = ( m_LinkerOptions.Find( "%1" ) || m_LinkerOptions.Find( "\"%1\"" ) );
        if ( hasInputToken == false )
        {
            Error::Error_1106_MissingRequiredToken( iter, function, ".LinkerOptions", "%1" );
            return false;
        }
        bool hasOutputToken = ( m_LinkerOptions.Find( "%2" ) || m_LinkerOptions.Find( "\"%2\"" ) );
        if ( hasOutputToken == false )
        {
            Error::Error_1106_MissingRequiredToken( iter, function, ".LinkerOptions", "%2" );
            return false;
        }
    }

    // Standard library dependencies
    Dependencies libraries( 64, true );
    for ( const AString & library : m_Libraries )
    {
        if ( DependOnNode( nodeGraph, iter, function, library, libraries ) == false )
        {
            return false; // DependOnNode will have emitted an error
        }
    }

    // Assembly Resources
    Dependencies assemblyResources( 32, true );
    if ( !function->GetNodeList( nodeGraph, iter, ".LinkerAssemblyResources", assemblyResources, false ) ) // TODO:B Use m_LinkerAssemblyResources directly
    {
        return false; // GetNodeList will have emitted error
    }

    // get inputs not passed through 'LibraryNodes' (i.e. directly specified on the cmd line)
    Dependencies otherLibraryNodes( 64, true );
    if ( ( m_Flags & ( LinkerNode::LINK_FLAG_MSVC | LinkerNode::LINK_FLAG_GCC | LinkerNode::LINK_FLAG_SNC | LinkerNode::LINK_FLAG_ORBIS_LD | LinkerNode::LINK_FLAG_GREENHILLS_ELXR | LinkerNode::LINK_FLAG_CODEWARRIOR_LD ) ) != 0 )
    {
        const bool msvcStyle = ( ( m_Flags & LinkerNode::LINK_FLAG_MSVC ) == LinkerNode::LINK_FLAG_MSVC );
        if ( !GetOtherLibraries( nodeGraph, iter, function, m_LinkerOptions, otherLibraryNodes, msvcStyle ) )
        {
            return false; // will have emitted error
        }
    }

    // Handle LinkerStampExe
    Node * linkerStampExeNode = nullptr;
    if ( m_LinkerStampExe.IsEmpty() == false )
    {
        if ( !function->GetFileNode( nodeGraph, iter, linkerStampExeNode, ".LinkerStampExe" ) ) // TODO: Use m_LinkerStampExe property
        {
            return false; // GetFileNode will have emitted an error
        }
    }

    // Store all dependencies
    m_StaticDependencies.SetCapacity( 1 + // for .Linker
                                      libraries.GetSize() +
                                      assemblyResources.GetSize() +
                                      otherLibraryNodes.GetSize() +
                                      ( linkerStampExeNode ? 1 : 0 ) );
    m_StaticDependencies.Append( Dependency( linkerExeNode ) );
    m_StaticDependencies.Append( libraries );
    m_AssemblyResourcesStartIndex = (uint32_t)m_StaticDependencies.GetSize();
    m_StaticDependencies.Append( assemblyResources );
    m_AssemblyResourcesNum = (uint32_t)assemblyResources.GetSize();
    m_StaticDependencies.Append( otherLibraryNodes );
    if ( linkerStampExeNode )
    {
        m_StaticDependencies.Append( Dependency( linkerStampExeNode ) );
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
LinkerNode::~LinkerNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult LinkerNode::DoBuild( Job * job )
{
    DoPreLinkCleanup();

    // Make sure the implib output directory exists
    if (m_ImportLibName.IsEmpty() == false)
    {
        AStackString<> cleanPath;
        NodeGraph::CleanPath(m_ImportLibName, cleanPath);

        if (EnsurePathExistsForFile(cleanPath) == false)
        {
            // EnsurePathExistsForFile will have emitted error
            return NODE_RESULT_FAILED;
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

    // we retry if linker crashes
    uint32_t attempt( 0 );

    for (;;)
    {
        ++attempt;

        // spawn the process
        Process p( FBuild::Get().GetAbortBuildPointer() );
        bool spawnOK = p.Spawn( m_Linker.Get(),
                                fullArgs.GetFinalArgs().Get(),
                                workingDir,
                                environment );

        if ( !spawnOK )
        {
            if ( p.HasAborted() )
            {
                return NODE_RESULT_FAILED;
            }

            FLOG_ERROR( "Failed to spawn process '%s' for %s creation for '%s'", m_Linker.Get(), GetDLLOrExe(), GetName().Get() );
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
        if ( p.HasAborted() )
        {
            return NODE_RESULT_FAILED;
        }

        // did the executable fail?
        if ( result != 0 )
        {
            // Handle bugs in the MSVC linker
            if ( GetFlag( LINK_FLAG_MSVC ) && ( attempt == 1 ) )
            {
                // Did the linker have an ICE (crash) (LNK1000)?
                if ( result == 1000 )
                {
                    FLOG_WARN( "FBuild: Warning: Linker crashed (LNK1000), retrying '%s'", GetName().Get() );
                    continue; // try again
                }

                // Did the linker have an "unexpected PDB error" (LNK1318)?
                // Example: "fatal error LNK1318: Unexpected PDB error; CORRUPT (13)"
                // (The linker or mspdbsrv.exe (as of VS2017) seems to have bugs which cause the PDB
                // to sometimes be corrupted when doing very large links, possibly because the linker
                // is running out of memory)
                if ( result == 1318 )
                {
                    FLOG_WARN( "FBuild: Warning: Linker corrupted the PDB (LNK1318), retrying '%s'", GetName().Get() );
                    continue; // try again
                }
            }

            if ( memOut.Get() )
            {
                job->ErrorPreformatted( memOut.Get() );
            }

            if ( memErr.Get() )
            {
                job->ErrorPreformatted( memErr.Get() );
            }

            // some other (genuine) linker failure
            FLOG_ERROR( "Failed to build %s (error %i) '%s'", GetDLLOrExe(), result, GetName().Get() );
            return NODE_RESULT_FAILED;
        }
        else
        {
            // If "warnings as errors" is enabled (/WX) we don't need to check
            // (since compilation will fail anyway, and the output will be shown)
            if ( GetFlag( LINK_FLAG_MSVC ) && !GetFlag( LINK_FLAG_WARNINGS_AS_ERRORS_MSVC ) )
            {
                HandleWarningsMSVC( job, GetName(), memOut.Get(), memOutSize );
            }
            break; // success!
        }
    }

    // post-link stamp step
    if ( m_LinkerStampExe.IsEmpty() == false )
    {
        const Node * linkerStampExe = m_StaticDependencies.End()[ -1 ].GetNode();
        EmitStampMessage();

        Process stampProcess( FBuild::Get().GetAbortBuildPointer() );
        bool spawnOk = stampProcess.Spawn( linkerStampExe->GetName().Get(),
                                           m_LinkerStampExeArgs.Get(),
                                           nullptr,     // working dir
                                           nullptr );   // env
        if ( spawnOk == false )
        {
            if ( stampProcess.HasAborted() )
            {
                return NODE_RESULT_FAILED;
            }

            FLOG_ERROR( "Failed to spawn process '%s' for '%s' stamping of '%s'", linkerStampExe->GetName().Get(), GetDLLOrExe(), GetName().Get() );
            return NODE_RESULT_FAILED;
        }

        // capture all of the stdout and stderr
        AutoPtr< char > memOut;
        AutoPtr< char > memErr;
        uint32_t memOutSize = 0;
        uint32_t memErrSize = 0;
        stampProcess.ReadAllData( memOut, &memOutSize, memErr, &memErrSize );
        ASSERT( !stampProcess.IsRunning() );

        // Get result
        int result = stampProcess.WaitForExit();
        if ( stampProcess.HasAborted() )
        {
            return NODE_RESULT_FAILED;
        }

        // did the executable fail?
        if ( result != 0 )
        {
            if ( memOut.Get() ) { FLOG_ERROR_DIRECT( memOut.Get() ); }
            if ( memErr.Get() ) { FLOG_ERROR_DIRECT( memErr.Get() ); }
            FLOG_ERROR( "Failed to stamp %s '%s' (error %i - '%s')", GetDLLOrExe(), GetName().Get(), result, m_LinkerStampExe.Get() );
            return NODE_RESULT_FAILED;
        }

        // success!
    }

    // record time stamp for next time
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    ASSERT( m_Stamp );

    return NODE_RESULT_OK;
}

// DoPreLinkCleanup
//------------------------------------------------------------------------------
void LinkerNode::DoPreLinkCleanup() const
{
    // only for Microsoft compilers
    if ( GetFlag( LINK_FLAG_MSVC ) == false )
    {
        return;
    }

    bool deleteFiles = false;
    if ( GetFlag( LINK_FLAG_INCREMENTAL ) )
    {
        if ( FBuild::Get().GetOptions().m_ForceCleanBuild )
        {
            deleteFiles = true; // force a full re-link
        }
    }
    else
    {
        // cleanup old ILK files and force PDB to be rebuilt
        // (old PDBs get large and dramatically slow link times)
        deleteFiles = true;
    }

    // Work around for VS2013/2015 bug where /showincludes doesn't work if cl.exe
    // thinks it can skip compilation (it outputs "Note: reusing persistent precompiled header %s")
    // If FASTBuild is building because we've not built before, then cleanup old files
    // to ensure VS2013/2015 /showincludes will work
    if ( GetStamp() == 0 )
    {
        deleteFiles = true;
    }

    if ( deleteFiles )
    {
        // output file
        FileIO::FileDelete( GetName().Get() );

        // .ilk
        const char * lastDot = GetName().FindLast( '.' );
        AStackString<> ilkName( GetName().Get(), lastDot ? lastDot : GetName().GetEnd() );
        ilkName += ".ilk";
        FileIO::FileDelete( ilkName.Get() );

        // .pdb - TODO: Handle manually specified /PDB
        AStackString<> pdbName( GetName().Get(), lastDot ? lastDot : GetName().GetEnd() );
        pdbName += ".pdb";
        FileIO::FileDelete( pdbName.Get() );
    }
}

// BuildArgs
//------------------------------------------------------------------------------
bool LinkerNode::BuildArgs( Args & fullArgs ) const
{
    PROFILE_FUNCTION

    // split into tokens
    Array< AString > tokens( 1024, true );
    m_LinkerOptions.Tokenize( tokens );

    const AString * const end = tokens.End();
    for ( const AString * it = tokens.Begin(); it!=end; ++it )
    {
        const AString & token = *it;

        // %1 -> InputFiles
        const char * found = token.Find( "%1" );
        if ( found )
        {
            AStackString<> pre( token.Get(), found );
            AStackString<> post( found + 2, token.GetEnd() );
            GetInputFiles( fullArgs, pre, post );
            fullArgs.AddDelimiter();
            continue;
        }

        // %2 -> OutputFile
        found = token.Find( "%2" );
        if ( found )
        {
            fullArgs += AStackString<>( token.Get(), found );
            fullArgs += m_Name;
            fullArgs += AStackString<>( found + 2, token.GetEnd() );
            fullArgs.AddDelimiter();
            continue;
        }

        if ( GetFlag( LINK_FLAG_MSVC ) )
        {
            // %3 -> AssemblyResources
            found = token.Find( "%3" );
            if ( found )
            {
                AStackString<> pre( token.Get(), found );
                AStackString<> post( found + 2, token.GetEnd() );
                GetAssemblyResourceFiles( fullArgs, pre, post );
                fullArgs.AddDelimiter();
                continue;
            }

            if ( IsStartOfLinkerArg_MSVC(token, "LIBPATH:") == true )
            {
                // get remainder of token after arg
                const char * valueStart = token.Get() + 8 + 1;
                const char * valueEnd = token.GetEnd();

                AStackString<> value;
                Args::StripQuotes( valueStart, valueEnd, value );

                AStackString<> cleanValue;
                NodeGraph::CleanPath( value, cleanValue, false );

                fullArgs += token[0]; // reuse whichever prefix, / or -
                fullArgs += "LIBPATH:\"";
                fullArgs += cleanValue;
                fullArgs += '\"';
                fullArgs.AddDelimiter();

                continue;
            }
        }

        // untouched token
        fullArgs += token;
        fullArgs.AddDelimiter();
    }

    // orbis-ld.exe requires escaped slashes inside response file
    if ( GetFlag( LINK_FLAG_ORBIS_LD ) )
    {
        fullArgs.SetEscapeSlashesInResponseFile();
    }

    // Handle all the special needs of args
    if ( fullArgs.Finalize( m_Linker, GetName(), CanUseResponseFile() ) == false )
    {
        return false; // Finalize will have emitted an error
    }

    return true;
}

// GetInputFiles
//------------------------------------------------------------------------------
void LinkerNode::GetInputFiles( Args & fullArgs, const AString & pre, const AString & post ) const
{
    // Regular inputs are after linker and before AssemblyResources
    const Dependency * start = m_StaticDependencies.Begin() + 1; // Skip first item which is linker exe
    const Dependency * end = m_StaticDependencies.Begin() + m_AssemblyResourcesStartIndex;
    for ( const Dependency * i = start; i != end; ++i )
    {
        Node * n( i->GetNode() );
        GetInputFiles( n, fullArgs, pre, post );
    }
}

// GetInputFiles
//------------------------------------------------------------------------------
void LinkerNode::GetInputFiles( Node * n, Args & fullArgs, const AString & pre, const AString & post ) const
{
    if ( n->GetType() == Node::LIBRARY_NODE )
    {
        const bool linkObjectsInsteadOfLibs = m_LinkerLinkObjects;

        if ( linkObjectsInsteadOfLibs )
        {
            LibraryNode * ln = n->CastTo< LibraryNode >();
            ln->GetInputFiles( fullArgs, pre, post, linkObjectsInsteadOfLibs );
        }
        else
        {
            // not building a DLL, so link the lib directly
            fullArgs += pre;
            fullArgs += n->GetName();
            fullArgs += post;
        }
    }
    else if ( n->GetType() == Node::OBJECT_LIST_NODE )
    {
        const bool linkObjectsInsteadOfLibs = m_LinkerLinkObjects;

        ObjectListNode * ol = n->CastTo< ObjectListNode >();
        ol->GetInputFiles( fullArgs, pre, post, linkObjectsInsteadOfLibs );
    }
    else if ( n->GetType() == Node::DLL_NODE )
    {
        // for a DLL, link to the import library
        DLLNode * dllNode = n->CastTo< DLLNode >();
        AStackString<> importLibName;
        dllNode->GetImportLibName( importLibName );
        fullArgs += pre;
        fullArgs += importLibName;
        fullArgs += post;
    }
    else if ( n->GetType() == Node::COPY_FILE_NODE )
    {
        CopyFileNode * copyNode = n->CastTo< CopyFileNode >();
        Node * srcNode = copyNode->GetSourceNode();
        GetInputFiles( srcNode, fullArgs, pre, post );
    }
    else
    {
        // link anything else directly
        fullArgs += pre;
        fullArgs += n->GetName();
        fullArgs += post;
    }

    fullArgs.AddDelimiter();
}

// GetAssemblyResourceFiles
//------------------------------------------------------------------------------
void LinkerNode::GetAssemblyResourceFiles( Args & fullArgs, const AString & pre, const AString & post ) const
{
    const Dependency * start = m_StaticDependencies.Begin() + m_AssemblyResourcesStartIndex;
    const Dependency * end = start + m_AssemblyResourcesNum;
    for ( const Dependency * i = start; i != end; ++i )
    {
        Node * n( i->GetNode() );

        if ( n->GetType() == Node::OBJECT_LIST_NODE )
        {
            ObjectListNode * oln = n->CastTo< ObjectListNode >();
            oln->GetInputFiles( fullArgs, pre, post, false );
            continue;
        }

        if ( n->GetType() == Node::LIBRARY_NODE )
        {
            LibraryNode * ln = n->CastTo< LibraryNode >();
            ln->GetInputFiles( fullArgs, pre, post, false );
            continue;
        }

        fullArgs += pre;
        fullArgs += n->GetName();
        fullArgs += post;
        fullArgs.AddDelimiter();
    }
}

// DetermineLinkerTypeFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t LinkerNode::DetermineLinkerTypeFlags(const AString & linkerType, const AString & linkerName)
{
    uint32_t flags = 0;

    if ( linkerType.IsEmpty() || ( linkerType == "auto" ))
    {
        // Detect based upon linker executable name
        if ( ( linkerName.EndsWithI( "link.exe" ) ) ||
            ( linkerName.EndsWithI( "link" ) ) )
        {
            flags |= LinkerNode::LINK_FLAG_MSVC;
        }
        else if ( ( linkerName.EndsWithI( "gcc.exe" ) ) ||
            ( linkerName.EndsWithI( "gcc" ) ) )
        {
            flags |= LinkerNode::LINK_FLAG_GCC;
        }
        else if ( ( linkerName.EndsWithI( "ps3ppuld.exe" ) ) ||
            ( linkerName.EndsWithI( "ps3ppuld" ) ) )
        {
            flags |= LinkerNode::LINK_FLAG_SNC;
        }
        else if ( ( linkerName.EndsWithI( "orbis-ld.exe" ) ) ||
            ( linkerName.EndsWithI( "orbis-ld" ) ) )
        {
            flags |= LinkerNode::LINK_FLAG_ORBIS_LD;
        }
        else if ( ( linkerName.EndsWithI( "elxr.exe" ) ) ||
            ( linkerName.EndsWithI( "elxr" ) ) )
        {
            flags |= LinkerNode::LINK_FLAG_GREENHILLS_ELXR;
        }
        else if ( ( linkerName.EndsWithI( "mwldeppc.exe" ) ) ||
            ( linkerName.EndsWithI( "mwldeppc." ) ) )
        {
            flags |= LinkerNode::LINK_FLAG_CODEWARRIOR_LD;
        }
    }
    else
    {
        if ( linkerType == "msvc" )
        {
            flags |= LinkerNode::LINK_FLAG_MSVC;
        }
        else if ( linkerType == "gcc" )
        {
            flags |= LinkerNode::LINK_FLAG_GCC;
        }
        else if ( linkerType == "snc-ps3" )
        {
            flags |= LinkerNode::LINK_FLAG_SNC;
        }
        else if ( linkerType == "clang-orbis" )
        {
            flags |= LinkerNode::LINK_FLAG_ORBIS_LD;
        }
        else if ( linkerType == "greenhills-exlr" )
        {
            flags |= LinkerNode::LINK_FLAG_GREENHILLS_ELXR;
        }
        else if ( linkerType == "codewarrior-ld" )
        {
            flags |= LinkerNode::LINK_FLAG_CODEWARRIOR_LD;
        }
    }

    return flags;
}

// DetermineFlags
//------------------------------------------------------------------------------
/*static*/ uint32_t LinkerNode::DetermineFlags( const AString & linkerType, const AString & linkerName, const AString & args )
{
    uint32_t flags = DetermineLinkerTypeFlags( linkerType, linkerName );

    if ( flags & LINK_FLAG_MSVC )
    {
        // Parse args for some other flags
        Array< AString > tokens;
        args.Tokenize( tokens );

        bool debugFlag = false;
        bool incrementalFlag = false;
        bool incrementalNoFlag = false;
        bool optREFFlag = false;
        bool optICFFlag = false;
        bool optLBRFlag = false;
        bool orderFlag = false;

        const AString * const end = tokens.End();
        for ( const AString * it=tokens.Begin(); it!=end; ++it )
        {
            const AString & token = *it;
            if ( IsLinkerArg_MSVC( token, "DLL" ) )
            {
                flags |= LinkerNode::LINK_FLAG_DLL;
                continue;
            }

            if ( IsLinkerArg_MSVC( token, "DEBUG" ) )
            {
                debugFlag = true;
                continue;
            }

            if ( IsLinkerArg_MSVC( token, "INCREMENTAL" ) )
            {
                incrementalFlag = true;
                continue;
            }

            if ( IsLinkerArg_MSVC( token, "INCREMENTAL:NO" ) )
            {
                incrementalNoFlag = true;
                continue;
            }

            if ( IsLinkerArg_MSVC( token, "WX" ) )
            {
                flags |= LinkerNode::LINK_FLAG_WARNINGS_AS_ERRORS_MSVC;
                continue;
            }

            if ( IsStartOfLinkerArg_MSVC( token, "OPT" ) )
            {
                if ( token.FindI( "REF" ) )
                {
                    optREFFlag = true;
                }

                if ( token.FindI( "ICF" ) )
                {
                    optICFFlag = true;
                }

                if ( token.FindI( "LBR" ) )
                {
                    optLBRFlag = true;
                }

                continue;
            }

            if ( IsStartOfLinkerArg_MSVC( token, "ORDER" ) )
            {
                orderFlag = true;
                continue;
            }
        }

        // Determine incremental linking status
        bool usingIncrementalLinking = false; // false by default

        // these options enable incremental linking
        if ( debugFlag || incrementalFlag )
        {
            usingIncrementalLinking = true;
        }

        // these options disable incremental linking
        if ( incrementalNoFlag || optREFFlag || optICFFlag || optLBRFlag || orderFlag )
        {
            usingIncrementalLinking = false;
        }

        if ( usingIncrementalLinking )
        {
            flags |= LINK_FLAG_INCREMENTAL;
        }
    }
    else
    {
        // Parse args for some other flags
        Array< AString > tokens;
        args.Tokenize( tokens );

        const AString * const end = tokens.End();
        for ( const AString * it=tokens.Begin(); it!=end; ++it )
        {
            const AString & token = *it;
            if ( ( token == "-shared" ) || ( token == "-dynamiclib" ) || ( token == "--oformat=prx" ) )
            {
                flags |= LinkerNode::LINK_FLAG_DLL;
                continue;
            }
        }
    }

    return flags;
}

// IsLinkerArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool LinkerNode::IsLinkerArg_MSVC( const AString & token, const char * arg )
{
    ASSERT( token.IsEmpty() == false );

    // MSVC Linker args can start with - or /
    if ( ( token[0] != '/' ) && ( token[0] != '-' ) )
    {
        return false;
    }

    // Length check to early out
    const size_t argLen = AString::StrLen( arg );
    if ( ( token.GetLength() - 1 ) != argLen )
    {
        return false; // token is too short or too long
    }

    // MSVC Linker args are case-insensitive
    return token.EndsWithI( arg );
}

// IsStartOfLinkerArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool LinkerNode::IsStartOfLinkerArg_MSVC( const AString & token, const char * arg )
{
    ASSERT( token.IsEmpty() == false );

    // MSVC Linker args can start with - or /
    if ( ( token[0] != '/' ) && ( token[0] != '-' ) )
    {
        return false;
    }

    // Length check to early out
    const size_t argLen = AString::StrLen( arg );
    if ( ( token.GetLength() - 1 ) < argLen )
    {
        return false; // token is too short
    }

    // MSVC Linker args are case-insensitive
    return ( AString::StrNCmpI( token.Get() + 1, arg, argLen ) == 0 );
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void LinkerNode::EmitCompilationMessage( const Args & fullArgs ) const
{
    AStackString<> output;
    output += GetDLLOrExe();
    output += ": ";
    output += GetName();
    output += '\n';
    if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
    {
        output += m_Linker;
        output += ' ';
        output += fullArgs.GetRawArgs();
        output += '\n';
    }
    FLOG_BUILD_DIRECT( output.Get() );
}

// EmitStampMessage
//------------------------------------------------------------------------------
void LinkerNode::EmitStampMessage() const
{
    ASSERT( m_LinkerStampExe.IsEmpty() == false );
    const Node * linkerStampExe = m_StaticDependencies.End()[ -1 ].GetNode();

    AStackString<> output;
    output += "Stamp: ";
    output += GetName();
    output += '\n';
    if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
    {
        output += linkerStampExe->GetName();
        output += ' ';
        output += m_LinkerStampExeArgs;
        output += '\n';
    }
    FLOG_BUILD_DIRECT( output.Get() );
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void LinkerNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

// CanUseResponseFile
//------------------------------------------------------------------------------
bool LinkerNode::CanUseResponseFile() const
{
    #if defined( __WINDOWS__ )
        return ( GetFlag( LINK_FLAG_MSVC ) || GetFlag( LINK_FLAG_GCC ) || GetFlag( LINK_FLAG_SNC ) || GetFlag( LINK_FLAG_ORBIS_LD ) || GetFlag( LINK_FLAG_GREENHILLS_ELXR ) || GetFlag( LINK_FLAG_CODEWARRIOR_LD ) );
    #else
        return false;
    #endif
}

// GetImportLibName
//------------------------------------------------------------------------------
void LinkerNode::GetImportLibName( const AString & args, AString & importLibName ) const
{
    // split to individual tokens
    Array< AString > tokens;
    args.Tokenize( tokens );

    const AString * const end = tokens.End();
    for ( const AString * it = tokens.Begin(); it != end; ++it )
    {
        if ( LinkerNode::IsStartOfLinkerArg_MSVC( *it, "IMPLIB:" ) )
        {
            const char * impStart = it->Get() + 8;
            const char * impEnd = it->GetEnd();

            // if token is exactly /IMPLIB: then value is next token
            if ( impStart == impEnd )
            {
                ++it;
                // handle missing next value
                if ( it == end )
                {
                    return; // we just pretend it doesn't exist and let the linker complain
                }

                impStart = it->Get();
                impEnd = it->GetEnd();
            }

            Args::StripQuotes( impStart, impEnd, importLibName );
        }
    }
}

// GetOtherLibraries
//------------------------------------------------------------------------------
bool LinkerNode::GetOtherLibraries( NodeGraph & nodeGraph,
                                    const BFFIterator & iter,
                                    const Function * function,
                                    const AString & args,
                                    Dependencies & otherLibraries,
                                    bool msvc ) const
{
    // split to individual tokens
    Array< AString > tokens;
    args.Tokenize( tokens );

    bool ignoreAllDefaultLibs = false;
    Array< AString > defaultLibsToIgnore( 8, true );
    Array< AString > defaultLibs( 16, true );
    Array< AString > libs( 16, true );
    Array< AString > dashlLibs( 16, true );
    Array< AString > libPaths( 16, true );
    Array< AString > envLibPaths( 32, true );

    // extract lib path from system if present
    AStackString< 1024 > libVar;
    if ( Env::GetEnvVariable( "LIB", libVar ) )
    {
        libVar.Tokenize( envLibPaths, ';' );
    }

    const AString * const end = tokens.End();
    for ( const AString * it = tokens.Begin(); it != end; ++it )
    {
        const AString & token = *it;

        // MSVC style
        if ( msvc )
        {
            // /NODEFAULTLIB
            if ( LinkerNode::IsLinkerArg_MSVC( token, "NODEFAULTLIB" ) )
            {
                ignoreAllDefaultLibs = true;
                continue;
            }

            // /NODEFAULTLIB:
            if ( GetOtherLibsArg( "NODEFAULTLIB:", defaultLibsToIgnore, it, end, false, msvc ) )
            {
                continue;
            }

            // /DEFAULTLIB:
            if ( GetOtherLibsArg( "DEFAULTLIB:", defaultLibs, it, end, false, msvc ) )
            {
                continue;
            }

            // /LIBPATH:
            if ( GetOtherLibsArg( "LIBPATH:", libPaths, it, end, true, msvc ) ) // true = canonicalize path
            {
                continue;
            }

            // some other linker argument?
            if ( token.BeginsWith( '/' ) || token.BeginsWith( '-' ) )
            {
                continue;
            }
        }

        // GCC/SNC style
        if ( !msvc )
        {
            // We don't need to check for this, as there is no default lib passing on
            // the cmd line.
            // -nodefaultlibs
            //if ( token == "-nodefaultlibs" )
            //{
            //  ignoreAllDefaultLibs = true;
            //  continue;
            //}

            // -L (lib path)
            if ( GetOtherLibsArg( "L", libPaths, it, end, false, msvc ) )
            {
                continue;
            }

            // -l (lib)
            if ( GetOtherLibsArg( "l", dashlLibs, it, end, false, msvc ) )
            {
                continue;
            }

            // some other linker argument?
            if ( token.BeginsWith( '-' ) )
            {
                continue;
            }
        }

        // build time substitution?
        if ( token.BeginsWith( '%' ) ||     // %1
             token.BeginsWith( "'%" ) ||    // '%1'
             token.BeginsWith( "\"%" ) )    // "%1"
        {
            continue;
        }

        // anything left is an input to the linker
        AStackString<> libName;
        Args::StripQuotes( token.Get(), token.GetEnd(), libName );
        if ( token.IsEmpty() == false )
        {
            libs.Append( libName );
        }
    }

    // filter default libs
    if ( ignoreAllDefaultLibs )
    {
        // filter all default libs
        defaultLibs.Clear();
    }
    else
    {
        // filter specifically listed default libs
        const AString * const endI = defaultLibsToIgnore.End();
        for ( const AString * itI = defaultLibsToIgnore.Begin(); itI != endI; ++itI )
        {
            const AString * const endD = defaultLibs.End();
            for ( AString * itD = defaultLibs.Begin(); itD != endD; ++itD )
            {
                if ( itI->CompareI( *itD ) == 0 )
                {
                    defaultLibs.Erase( itD );
                    break;
                }
            }
        }
    }

    if ( !msvc )
    {
        // convert -l<name> style libs to lib<name>.a
        const AString * const endDL = dashlLibs.End();
        for ( const AString * itDL = dashlLibs.Begin(); itDL != endDL; ++itDL )
        {
            AStackString<> libName;
            libName += "lib";
            libName += *itDL;
            libName += ".a";
            libs.Append( libName );
        }
    }

    // any remaining default libs are treated the same as libs
    libs.Append( defaultLibs );

    // use Environment libpaths if found (but used after LIBPATH provided ones)
    libPaths.Append( envLibPaths );

    // convert libs to nodes
    const AString * const endL = libs.End();
    for ( const AString * itL = libs.Begin(); itL != endL; ++itL )
    {
        bool found = false;

        // is the file a full path?
        if ( ( itL->GetLength() > 2 ) && ( (*itL)[ 1 ] == ':' ) )
        {
            // check file exists in current location
            if ( !GetOtherLibrary( nodeGraph, iter, function, otherLibraries, AString::GetEmpty(), *itL, found ) )
            {
                return false; // GetOtherLibrary will have emitted error
            }
        }
        else
        {
            // check each libpath
            const AString * const endP = libPaths.End();
            for ( const AString * itP = libPaths.Begin(); itP != endP; ++itP )
            {
                if ( !GetOtherLibrary( nodeGraph, iter, function, otherLibraries, *itP, *itL, found ) )
                {
                    return false; // GetOtherLibrary will have emitted error
                }

                if ( found )
                {
                    break;
                }

                // keep searching lib paths...
            }
        }

        // file does not exist on disk, and there is no rule to build it
        // Don't complain about this, because:
        //  a) We may be parsing rules on another OS (i.e. parsing Linux rules on Windows)
        //  b) User may have filtered some libs for platforms they don't care about (i.e. libs
        //     for PS4 on a PC developer's machine on a cross-platform team)
        // If the file is actually needed, the linker will emit an error during link-time.
    }

    return true;
}

// GetOtherLibrary
//------------------------------------------------------------------------------
bool LinkerNode::GetOtherLibrary( NodeGraph & nodeGraph,
                                  const BFFIterator & iter,
                                  const Function * function,
                                  Dependencies & libs,
                                  const AString & path,
                                  const AString & lib,
                                  bool & found ) const
{
    found = false;

    AStackString<> potentialNodeName( path );
    if ( !potentialNodeName.IsEmpty() )
    {
        PathUtils::EnsureTrailingSlash( potentialNodeName );
    }
    potentialNodeName += lib;
    AStackString<> potentialNodeNameClean;
    NodeGraph::CleanPath( potentialNodeName, potentialNodeNameClean );

    // see if a node already exists
    Node * node = nodeGraph.FindNode( potentialNodeNameClean );
    if ( node )
    {
        // aliases not supported - must point to something that provides a file
        if ( node->IsAFile() == false )
        {
            Error::Error_1103_NotAFile( iter, function, ".LinkerOptions", potentialNodeNameClean, node->GetType() );
            return false;
        }

        // found existing node
        libs.Append( Dependency( node ) );
        found = true;
        return true; // no error
    }

    // see if the file exists on disk at this location
    if ( FileIO::FileExists( potentialNodeNameClean.Get() ) )
    {
        node = nodeGraph.CreateFileNode( potentialNodeNameClean );
        libs.Append( Dependency( node ) );
        found = true;
        FLOG_INFO( "Additional library '%s' assumed to be '%s'\n", lib.Get(), potentialNodeNameClean.Get() );
        return true; // no error
    }

    return true; // no error
}


// GetOtherLibsArg
//------------------------------------------------------------------------------
/*static*/ bool LinkerNode::GetOtherLibsArg( const char * arg,
                                             Array< AString > & list,
                                             const AString * & it,
                                             const AString * const & end,
                                             bool canonicalizePath,
                                             bool isMSVC )
{
    // check for expected arg
    if ( isMSVC )
    {
        if ( LinkerNode::IsStartOfLinkerArg_MSVC( *it, arg ) == false )
        {
            return false; // not our arg, not consumed
        }
    }
    else
    {
        if ( it->BeginsWith( arg ) == false )
        {
            return false; // not our arg, not consumed
        }
    }

    // get remainder of token after arg
    const char * valueStart = it->Get() + AString::StrLen( arg ) + 1;
    const char * valueEnd = it->GetEnd();

    // if no remainder, arg value is next token
    if ( valueStart == valueEnd )
    {
        ++it;

        // no more tokens? (malformed input)
        if ( it == end )
        {
            // ignore this item and let the linker complain about that
            return true; // arg consumed
        }

        // use next token a value
        valueStart = it->Get();
        valueEnd = it->GetEnd();
    }

    // eliminate quotes
    AStackString<> value;
    Args::StripQuotes( valueStart, valueEnd, value );

    // store if useful
    if ( value.IsEmpty() == false )
    {
        if ( canonicalizePath )
        {
            AStackString<> cleanValue;
            NodeGraph::CleanPath( value, cleanValue );
            PathUtils::EnsureTrailingSlash( cleanValue );
            list.Append( cleanValue );
        }
        else
        {
            list.Append( value );
        }
    }

    return true; // arg consumed
}

// DependOnNode
//------------------------------------------------------------------------------
/*static*/ bool LinkerNode::DependOnNode( NodeGraph & nodeGraph,
                                          const BFFIterator & iter,
                                          const Function * function,
                                          const AString & nodeName,
                                          Dependencies & nodes )
{
    // silently ignore empty nodes
    if ( nodeName.IsEmpty() )
    {
        return true;
    }

    Node * node = nodeGraph.FindNode( nodeName );

    // does it exist?
    if ( node != nullptr )
    {
        // process it
        return DependOnNode( iter, function, node, nodes );
    }

    // node not found - create a new FileNode, assuming we are
    // linking against an externally built library
    node = nodeGraph.CreateFileNode( nodeName );
    nodes.Append( Dependency( node ) );
    return true;
}

// DependOnNode
//------------------------------------------------------------------------------
/*static*/ bool LinkerNode::DependOnNode( const BFFIterator & iter,
                                          const Function * function,
                                          Node * node,
                                          Dependencies & nodes )
{
    ASSERT( node );

    // a previously declared library?
    if ( node->GetType() == Node::LIBRARY_NODE )
    {
        // can link directly to it
        nodes.Append( Dependency( node ) );
        return true;
    }

    // a previously declared object list?
    if ( node->GetType() == Node::OBJECT_LIST_NODE )
    {
        // can link directly to it
        nodes.Append( Dependency( node ) );
        return true;
    }

    // a dll?
    if ( node->GetType() == Node::DLL_NODE )
    {
        // TODO:B Depend on import lib
        nodes.Append( Dependency( node, true ) ); // NOTE: Weak dependency
        return true;
    }

    // a previously declared external file?
    if ( node->GetType() == Node::FILE_NODE )
    {
        // can link directy against it
        nodes.Append( Dependency( node ) );
        return true;
    }

    // a file copy?
    if ( node->GetType() == Node::COPY_FILE_NODE )
    {
        // depend on copy - will use input at build time
        nodes.Append( Dependency( node ) );
        return true;
    }

    // an external executable?
    if ( node->GetType() == Node::EXEC_NODE )
    {
        // depend on ndoe - will use exe output at build time
        nodes.Append( Dependency( node ) );
        return true;
    }

    // a group (alias)?
    if ( node->GetType() == Node::ALIAS_NODE )
    {
        // handle all targets in alias
        AliasNode * an = node->CastTo< AliasNode >();
        const Dependencies & aliasNodeList = an->GetAliasedNodes();
        const Dependencies::Iter end = aliasNodeList.End();
        for ( Dependencies::Iter it = aliasNodeList.Begin();
              it != end;
              ++it )
        {
            if ( DependOnNode( iter, function, it->GetNode(), nodes ) == false )
            {
                return false; // something went wrong lower down
            }
        }
        return true; // all nodes in group handled ok
    }

    // don't know how to handle this type of node
    Error::Error_1005_UnsupportedNodeType( iter, function, "Libraries", node->GetName(), node->GetType() );
    return false;
}

//------------------------------------------------------------------------------
