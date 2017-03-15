// Node.cpp - base interface for dependency graph nodes
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "NodeGraph.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionSettings.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"

#include "AliasNode.h"
#include "CompilerNode.h"
#include "CopyDirNode.h"
#include "CopyFileNode.h"
#include "CSNode.h"
#include "DirectoryListNode.h"
#include "DLLNode.h"
#include "ExeNode.h"
#include "ExecNode.h"
#include "FileNode.h"
#include "LibraryNode.h"
#include "ObjectListNode.h"
#include "ObjectNode.h"
#include "RemoveDirNode.h"
#include "SLNNode.h"
#include "TestNode.h"
#include "UnityNode.h"
#include "VCXProjectNode.h"
#include "XCodeProjectNode.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"
#include "Core/Math/xxHash.h"
#include "Core/Mem/Mem.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/LevenshteinDistance.h"
#include "Core/Tracing/Tracing.h"

#include <string.h>

// Defines
//------------------------------------------------------------------------------

// Static Data
//------------------------------------------------------------------------------
/*static*/ uint32_t NodeGraph::s_BuildPassTag( 0 );

// CONSTRUCTOR
//------------------------------------------------------------------------------
NodeGraph::NodeGraph()
: m_AllNodes( 1024, true )
, m_NextNodeIndex( 0 )
, m_UsedFiles( 16, true )
{
    m_NodeMap = FNEW_ARRAY( Node *[NODEMAP_TABLE_SIZE] );
    memset( m_NodeMap, 0, sizeof( Node * ) * NODEMAP_TABLE_SIZE );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
NodeGraph::~NodeGraph()
{
    Array< Node * >::Iter i = m_AllNodes.Begin();
    Array< Node * >::Iter end = m_AllNodes.End();
    for ( ; i != end; ++i )
    {
        FDELETE ( *i );
    }

    FDELETE_ARRAY( m_NodeMap );
}

// Initialize
//------------------------------------------------------------------------------
/* static*/ NodeGraph * NodeGraph::Initialize( const char * bffFile,
                                               const char * nodeGraphDBFile )
{
    PROFILE_FUNCTION

    ASSERT( bffFile ); // must be supplied (or left as default)
    ASSERT( nodeGraphDBFile ); // must be supplied (or left as default)

    // Try to load the old DB
    NodeGraph * oldNG = FNEW( NodeGraph );
    LoadResult res = oldNG->Load( nodeGraphDBFile );

    // What happened?
    switch ( res )
    {
        case LoadResult::MISSING:
        {
            // Create a fresh DB by parsing the BFF
            FDELETE( oldNG );
            NodeGraph * newNG = FNEW( NodeGraph );
            if ( newNG->ParseFromRoot( bffFile ) == false )
            {
                FDELETE( newNG );
                return nullptr; // ParseFromRoot will have emitted an error
            }
            return newNG;
        }
        case LoadResult::LOAD_ERROR:
        {
            // Corrupt DB or other fatal problem
            FDELETE( oldNG );
            return nullptr;
        }
        case LoadResult::OK_BFF_CHANGED:
        {
            // Create a fresh DB by parsing the modified BFF
            NodeGraph * newNG = FNEW( NodeGraph );
            if ( newNG->ParseFromRoot( bffFile ) == false )
            {
                FDELETE( newNG );
                FDELETE( oldNG );
                return nullptr;
            }

            // TODO: Migrate old DB info to new DB
            FDELETE( oldNG );

            return newNG;
        }
        case LoadResult::OK:
        {
            // Nothing more to do
            return oldNG;
        }
    }

    ASSERT( false ); // Should not get here
    return nullptr;
}

// ParseFromRoot
//------------------------------------------------------------------------------
bool NodeGraph::ParseFromRoot( const char * bffFile )
{
    ASSERT( m_UsedFiles.IsEmpty() ); // NodeGraph cannot be recycled

    // open the configuration file
    FLOG_INFO( "Loading BFF '%s'", bffFile );
    FileStream bffStream;
    if ( bffStream.Open( bffFile ) == false )
    {
        // missing bff is a fatal problem
        FLOG_ERROR( "Failed to open BFF '%s'", bffFile );
        return false;
    }
    const uint64_t rootBFFTimeStamp = FileIO::GetFileLastWriteTime( AStackString<>( bffFile ) );

    // read entire config into memory
    uint32_t size = (uint32_t)bffStream.GetFileSize();
    AutoPtr< char > data( (char *)ALLOC( size + 1 ) ); // extra byte for null character sentinel
    if ( bffStream.Read( data.Get(), size ) != size )
    {
        FLOG_ERROR( "Error reading BFF '%s'", bffFile );
        return false;
    }
    const uint64_t rootBFFDataHash = xxHash::Calc64( data.Get(), size );

    // re-parse the BFF from scratch, clean build will result
    BFFParser bffParser( *this );
    data.Get()[ size ] = '\0'; // data passed to parser must be NULL terminated
    return bffParser.Parse( data.Get(), size, bffFile, rootBFFTimeStamp, rootBFFDataHash ); // pass size excluding sentinel
}

// Load
//------------------------------------------------------------------------------
NodeGraph::LoadResult NodeGraph::Load( const char * nodeGraphDBFile )
{
    // Open previously saved DB
    FileStream fs;
    if ( fs.Open( nodeGraphDBFile, FileStream::READ_ONLY ) == false )
    {
        return LoadResult::MISSING;
    }

    // Read it into memory to avoid lots of tiny disk accesses
    const size_t fileSize = (size_t)fs.GetFileSize();
    AutoPtr< char > memory( FNEW( char[ fileSize ] ) );
    if ( fs.ReadBuffer( memory.Get(), fileSize ) != fileSize )
    {
        return LoadResult::LOAD_ERROR;
    }
    ConstMemoryStream ms( memory.Get(), fileSize );

    // Load the Old DB
    return Load( ms, nodeGraphDBFile );
}

// Load
//------------------------------------------------------------------------------
NodeGraph::LoadResult NodeGraph::Load( IOStream & stream, const char * nodeGraphDBFile )
{
    bool compatibleDB = true;
    Array< UsedFile > usedFiles;
    if ( ReadHeaderAndUsedFiles( stream, nodeGraphDBFile, usedFiles, compatibleDB ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }

    // old or otherwise incompatible DB version?
    if ( !compatibleDB )
    {
        FLOG_WARN( "Database version has changed (clean build will occur)." );
        return LoadResult::OK_BFF_CHANGED;
    }

    // check if any files used have changed
    for ( size_t i=0; i<usedFiles.GetSize(); ++i )
    {
        const AString & fileName = usedFiles[ i ].m_FileName;
        const uint64_t timeStamp = FileIO::GetFileLastWriteTime( fileName );
        if ( timeStamp == usedFiles[ i ].m_TimeStamp )
        {
            continue; // timestamps match, no need to check hashes
        }

        FileStream fs;
        if ( fs.Open( fileName.Get(), FileStream::READ_ONLY ) == false )
        {
            FLOG_INFO( "BFF file '%s' missing or unopenable (reparsing will occur).", fileName.Get() );
            return LoadResult::OK_BFF_CHANGED; // not opening the file is not an error, it could be not needed anymore
        }

        const size_t size = (size_t)fs.GetFileSize();
        AutoPtr< void > mem( ALLOC( size ) );
        if ( fs.Read( mem.Get(), size ) != size )
        {
            return LoadResult::LOAD_ERROR; // error reading
        }

        const uint64_t dataHash = xxHash::Calc64( mem.Get(), size );
        if ( dataHash == usedFiles[ i ].m_DataHash )
        {
            // file didn't change, update stored timestamp to save time on the next run
            usedFiles[ i ].m_TimeStamp = timeStamp;
            continue;
        }

        FLOG_WARN( "BFF file '%s' has changed (reparsing will occur).", fileName.Get() );
        return LoadResult::OK_BFF_CHANGED;
    }

    m_UsedFiles = usedFiles;

    // TODO:C The serialization of these settings doesn't really belong here (not part of node graph)
    // cachepath
    AStackString<> cachePath;
    if ( stream.Read( cachePath ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }

    // cache plugin dll
    AStackString<> cachePluginDLL;
    if ( stream.Read( cachePluginDLL ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }

    // environment
    uint32_t envStringSize = 0;
    if ( stream.Read( envStringSize ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }
    AutoPtr< char > envString;
    AStackString<> libEnvVar;
    if ( envStringSize > 0 )
    {
        envString = ( (char *)ALLOC( envStringSize ) );
        if ( stream.Read( envString.Get(), envStringSize ) == false )
        {
            return LoadResult::LOAD_ERROR;
        }
        if ( stream.Read( libEnvVar ) == false )
        {
            return LoadResult::LOAD_ERROR;
        }
    }

    // imported environment variables
    uint32_t importedEnvironmentsVarsSize = 0;
    if ( stream.Read( importedEnvironmentsVarsSize ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }
    if ( importedEnvironmentsVarsSize > 0 )
    {
        AStackString<> varName;
        AStackString<> varValue;
        uint32_t savedVarHash = 0;
        uint32_t importedVarHash = 0;

        for ( uint32_t i = 0; i < importedEnvironmentsVarsSize; ++i )
        {
            if ( stream.Read( varName ) == false )
            {
                return LoadResult::LOAD_ERROR;
            }
            if ( stream.Read( savedVarHash ) == false )
            {
                return LoadResult::LOAD_ERROR;
            }

            bool optional = ( savedVarHash == 0 ); // a hash of 0 means the env var was missing when it was evaluated
            if ( FBuild::Get().ImportEnvironmentVar( varName.Get(), optional, varValue, importedVarHash ) == false )
            {
                // make sure the user knows why some things might re-build
                FLOG_WARN( "'%s' Environment variable was not found - BFF will be re-parsed\n", varName.Get() );
                return LoadResult::OK_BFF_CHANGED;
            }
            if ( importedVarHash != savedVarHash )
            {
                // make sure the user knows why some things might re-build
                FLOG_WARN( "'%s' Environment variable has changed - BFF will be re-parsed\n", varName.Get() );
                return LoadResult::OK_BFF_CHANGED;
            }
        }
    }

    // check if 'LIB' env variable has changed
    uint32_t libEnvVarHashInDB( 0 );
    if ( stream.Read( libEnvVarHashInDB ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }
    else
    {
        // If the Environment will be overriden, make sure we use the LIB from that
        const uint32_t libEnvVarHash = ( envStringSize > 0 ) ? xxHash::Calc32( libEnvVar ) : GetLibEnvVarHash();
        if ( libEnvVarHashInDB != libEnvVarHash )
        {
            // make sure the user knows why some things might re-build
            FLOG_WARN( "'%s' Environment variable has changed - BFF will be re-parsed\n", "LIB" );
            return LoadResult::OK_BFF_CHANGED;
        }
    }

    // worker list
    Array< AString > workerList( 0, true );
    if ( stream.Read( workerList ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }

    ASSERT( m_AllNodes.GetSize() == 0 );

    // Read nodes
    uint32_t numNodes;
    if ( stream.Read( numNodes ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }

    m_AllNodes.SetSize( numNodes );
    memset( m_AllNodes.Begin(), 0, numNodes * sizeof( Node * ) );
    for ( uint32_t i=0; i<numNodes; ++i )
    {
        if ( LoadNode( stream ) == false )
        {
            return LoadResult::LOAD_ERROR;
        }
    }

    // sanity check loading
    for ( size_t i=0; i<numNodes; ++i )
    {
        ASSERT( m_AllNodes[ i ] ); // each node was loaded
        ASSERT( m_AllNodes[ i ]->GetIndex() == i ); // index was correctly persisted
    }

    // Everything OK - propagate global settings
    //------------------------------------------------

    // Cache
    if ( cachePath.IsEmpty() == false ) // override environment only if not empty
    {
        FunctionSettings::SetCachePath( cachePath );
        FBuild::Get().SetCachePath( cachePath );
    }
    FBuild::Get().SetCachePluginDLL( cachePluginDLL );

    // Environment
    if ( envStringSize > 0 )
    {
        FBuild::Get().SetEnvironmentString( envString.Get(), envStringSize, libEnvVar );
    }

    // Workers
    FBuild::Get().SetWorkerList( workerList );

    return LoadResult::OK;
}

// LoadNode
//------------------------------------------------------------------------------
bool NodeGraph::LoadNode( IOStream & stream )
{
    // load index
    uint32_t nodeIndex( INVALID_NODE_INDEX );
    if ( stream.Read( nodeIndex ) == false )
    {
        return false;
    }

    // sanity check loading (each node saved once)
    ASSERT( m_AllNodes[ nodeIndex ] == nullptr );
    m_NextNodeIndex = nodeIndex;

    // load specifics (create node)
    Node * n = Node::Load( *this, stream );
    if ( n == nullptr )
    {
        return false;
    }

    // sanity check node index was correctly restored
    ASSERT( m_AllNodes[ nodeIndex ] == n );
    ASSERT( n->GetIndex() == nodeIndex );

    // load build time
    uint32_t lastTimeToBuild;
    if ( stream.Read( lastTimeToBuild ) == false )
    {
        return false;
    }
    n->SetLastBuildTime( lastTimeToBuild );

    return true;
}

// Save
//------------------------------------------------------------------------------
void NodeGraph::Save( IOStream & stream, const char* nodeGraphDBFile ) const
{
    // write header and version
    NodeGraphHeader header;
    stream.Write( (const void *)&header, sizeof( header ) );

    AStackString<> nodeGraphDBFileClean( nodeGraphDBFile );
    NodeGraph::CleanPath( nodeGraphDBFileClean );
    stream.Write( nodeGraphDBFileClean );

    // write used file
    uint32_t numUsedFiles = (uint32_t)m_UsedFiles.GetSize();
    stream.Write( numUsedFiles );

    for ( uint32_t i=0; i<numUsedFiles; ++i )
    {
        const AString & fileName = m_UsedFiles[ i ].m_FileName;
        uint32_t fileNameLen( fileName.GetLength() );
        stream.Write( fileNameLen );
        stream.Write( fileName.Get(), fileNameLen );
        uint64_t timeStamp( m_UsedFiles[ i ].m_TimeStamp );
        stream.Write( timeStamp );
        uint64_t dataHash( m_UsedFiles[ i ].m_DataHash );
        stream.Write( dataHash );
    }

    // TODO:C The serialization of these settings doesn't really belong here (not part of node graph)
    {
        // cache path
        stream.Write( FunctionSettings::GetCachePath() );
        stream.Write( FBuild::Get().GetCachePluginDLL() );

        // environment
        const uint32_t envStringSize = FBuild::Get().GetEnvironmentStringSize();
        stream.Write( envStringSize );
        if ( envStringSize > 0 )
        {
            const char * envString = FBuild::Get().GetEnvironmentString();
            stream.Write( envString, envStringSize );

            AStackString<> libEnvVar;
            FBuild::Get().GetLibEnvVar( libEnvVar );
            stream.Write( libEnvVar );
        }

        // imported environment variables
        const Array< FBuild::EnvironmentVarAndHash > & importedEnvironmentsVars = FBuild::Get().GetImportedEnvironmentVars();
        const uint32_t importedEnvironmentsVarsSize = static_cast<uint32_t>( importedEnvironmentsVars.GetSize() );
        ASSERT( importedEnvironmentsVarsSize == importedEnvironmentsVars.GetSize() );
        stream.Write( importedEnvironmentsVarsSize );
        for ( uint32_t i = 0; i < importedEnvironmentsVarsSize; ++i )
        {
            const FBuild::EnvironmentVarAndHash & varAndHash = importedEnvironmentsVars[i];
            stream.Write( varAndHash.GetName() );
            stream.Write( varAndHash.GetHash() );
        }

        // 'LIB' env var hash
        const uint32_t libEnvVarHash = GetLibEnvVarHash();
        stream.Write( libEnvVarHash );

        // worker list
        const Array< AString > & workerList = FBuild::Get().GetWorkerList();
        stream.Write( workerList );
    }

    // Write nodes
    size_t numNodes = m_AllNodes.GetSize();
    stream.Write( (uint32_t)numNodes );

    // save recursively
    Array< bool > savedNodeFlags( numNodes, false );
    savedNodeFlags.SetSize( numNodes );
    memset( savedNodeFlags.Begin(), 0, numNodes );
    for ( size_t i=0; i<numNodes; ++i )
    {
        SaveRecurse( stream, m_AllNodes[ i ], savedNodeFlags );
    }

    // sanity check saving
    for ( size_t i=0; i<numNodes; ++i )
    {
        ASSERT( savedNodeFlags[ i ] == true ); // each node was saved
    }
}

// SaveRecurse
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::SaveRecurse( IOStream & stream, Node * node, Array< bool > & savedNodeFlags )
{
    // ignore any already saved nodes
    uint32_t nodeIndex = node->GetIndex();
    ASSERT( nodeIndex != INVALID_NODE_INDEX );
    if ( savedNodeFlags[ nodeIndex ] )
    {
        return;
    }

    // Dependencies
    SaveRecurse( stream, node->GetPreBuildDependencies(), savedNodeFlags );
    SaveRecurse( stream, node->GetStaticDependencies(), savedNodeFlags );
    SaveRecurse( stream, node->GetDynamicDependencies(), savedNodeFlags );

    // save this node
    ASSERT( savedNodeFlags[ nodeIndex ] == false ); // sanity check recursion

    // save index
    stream.Write( nodeIndex );

    // save node specific data
    Node::Save( stream, node );

    // save build time
    uint32_t lastBuildTime = node->GetLastBuildTime();
    stream.Write( lastBuildTime );

    savedNodeFlags[ nodeIndex ] = true; // mark as saved
}

// SaveRecurse
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::SaveRecurse( IOStream & stream, const Dependencies & dependencies, Array< bool > & savedNodeFlags )
{
    const Dependency * const end = dependencies.End();
    for ( const Dependency * it = dependencies.Begin(); it != end; ++it )
    {
        Node * n = it->GetNode();
        SaveRecurse( stream, n, savedNodeFlags );
    }
}

// FindNode (AString &)
//------------------------------------------------------------------------------
Node * NodeGraph::FindNode( const AString & nodeName ) const
{
    // try to find node 'as is'
    Node * n = FindNodeInternal( nodeName );
    if ( n )
    {
        return n;
    }

    // the expanding to a full path
    AStackString< 1024 > fullPath;
    CleanPath( nodeName, fullPath );
    return FindNodeInternal( fullPath );
}

// GetNodeByIndex
//------------------------------------------------------------------------------
Node * NodeGraph::GetNodeByIndex( size_t index ) const
{
    Node * n = m_AllNodes[ index ];
    ASSERT( n );
    return n;
}

//GetNodeCount
//-----------------------------------------------------------------------------
size_t NodeGraph::GetNodeCount() const
{
    return m_AllNodes.GetSize();
}

// CreateCopyFileNode
//------------------------------------------------------------------------------
CopyFileNode * NodeGraph::CreateCopyFileNode( const AString & dstFileName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( dstFileName ) );

    CopyFileNode * node = FNEW( CopyFileNode() );
    node->SetName( dstFileName );
    AddNode( node );
    return node;
}

// CreateCopyDirNode
//------------------------------------------------------------------------------
CopyDirNode * NodeGraph::CreateCopyDirNode( const AString & nodeName )
{
    ASSERT( Thread::IsMainThread() );

    CopyDirNode * node = FNEW( CopyDirNode() );
    node->SetName( nodeName );
    AddNode( node );
    return node;
}

// CreateRemoveDirNode
//------------------------------------------------------------------------------
RemoveDirNode * NodeGraph::CreateRemoveDirNode( const AString & nodeName,
                                                Dependencies & staticDeps,
                                                const Dependencies & preBuildDependencies )
{
    ASSERT( Thread::IsMainThread() );

    RemoveDirNode * node = FNEW( RemoveDirNode( nodeName, staticDeps, preBuildDependencies ) );
    AddNode( node );
    return node;
}

// CreateExecNode
//------------------------------------------------------------------------------
ExecNode * NodeGraph::CreateExecNode( const AString & dstFileName,
                                      const Dependencies & inputFiles,
                                      FileNode * executable,
                                      const AString & arguments,
                                      const AString & workingDir,
                                      int32_t expectedReturnCode,
                                      const Dependencies & preBuildDependencies,
                                      bool useStdOutAsOutput )
{
    ASSERT( Thread::IsMainThread() );

    AStackString< 512 > fullPath;
    CleanPath( dstFileName, fullPath );

    ExecNode * node = FNEW( ExecNode( fullPath, inputFiles, executable, arguments, workingDir, expectedReturnCode, preBuildDependencies, useStdOutAsOutput ) );
    AddNode( node );
    return node;
}

// CreateFileNode
//------------------------------------------------------------------------------
FileNode * NodeGraph::CreateFileNode( const AString & fileName, bool cleanPath )
{
    ASSERT( Thread::IsMainThread() );

    FileNode * node;

    if ( cleanPath )
    {
        AStackString< 512 > fullPath;
        CleanPath( fileName, fullPath );
        node = FNEW( FileNode( fullPath ) );
    }
    else
    {
        node = FNEW( FileNode( fileName ) );
    }

    AddNode( node );
    return node;
}

// CreateDirectoryListNode
//------------------------------------------------------------------------------
DirectoryListNode * NodeGraph::CreateDirectoryListNode( const AString & name,
                                                        const AString & path,
                                                        const Array< AString > * patterns,
                                                        bool recursive,
                                                        const Array< AString > & excludePaths,
                                                        const Array< AString > & filesToExclude,
                                                        const Array< AString > & excludePatterns )
{
    ASSERT( Thread::IsMainThread() );

    // NOTE: DirectoryListNode assumes valid values from here
    // and will assert as such (so we don't check here)

    DirectoryListNode * node = FNEW( DirectoryListNode( name, path, patterns, recursive, excludePaths, filesToExclude, excludePatterns ) );
    AddNode( node );
    return node;
}

// CreateLibraryNode
//------------------------------------------------------------------------------
LibraryNode * NodeGraph::CreateLibraryNode( const AString & libraryName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( libraryName ) );

    LibraryNode * node = FNEW( LibraryNode() );
    node->SetName( libraryName );
    AddNode( node );
    return node;
}

// CreateObjectNode
//------------------------------------------------------------------------------
ObjectNode * NodeGraph::CreateObjectNode( const AString & objectName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( objectName ) );

    ObjectNode * node = FNEW( ObjectNode() );
    node->SetName( objectName );
    AddNode( node );
    return node;
}

// CreateAliasNode
//------------------------------------------------------------------------------
AliasNode * NodeGraph::CreateAliasNode( const AString & aliasName )
{
    ASSERT( Thread::IsMainThread() );

    AliasNode * node = FNEW( AliasNode() );
    node->SetName( aliasName );
    AddNode( node );
    return node;
}

// CreateDLLNode
//------------------------------------------------------------------------------
DLLNode * NodeGraph::CreateDLLNode( const AString & linkerOutputName,
                                    const Dependencies & inputLibraries,
                                    const Dependencies & otherLibraries,
                                    const AString & linkerType,
                                    const AString & linker,
                                    const AString & linkerArgs,
                                    uint32_t flags,
                                    const Dependencies & assemblyResources,
                                    const AString & importLibName,
                                    Node * linkerStampExe,
                                    const AString & linkerStampExeArgs )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( inputLibraries.IsEmpty() == false );

    AStackString< 1024 > fullPath;
    CleanPath( linkerOutputName, fullPath );

    DLLNode * node = FNEW( DLLNode( fullPath,
                                  inputLibraries,
                                  otherLibraries,
                                  linkerType,
                                  linker,
                                  linkerArgs,
                                  flags,
                                  assemblyResources,
                                  importLibName,
                                  linkerStampExe,
                                  linkerStampExeArgs ) );
    AddNode( node );
    return node;
}

// CreateExeNode
//------------------------------------------------------------------------------
ExeNode * NodeGraph::CreateExeNode( const AString & linkerOutputName,
                                    const Dependencies & inputLibraries,
                                    const Dependencies & otherLibraries,
                                    const AString & linkerType,
                                    const AString & linker,
                                    const AString & linkerArgs,
                                    uint32_t flags,
                                    const Dependencies & assemblyResources,
                                    const AString & importLibName,
                                    Node * linkerStampExe,
                                    const AString & linkerStampExeArgs )
{
    ASSERT( Thread::IsMainThread() );

    AStackString< 1024 > fullPath;
    CleanPath( linkerOutputName, fullPath );

    ExeNode * node = FNEW( ExeNode( fullPath,
                                  inputLibraries,
                                  otherLibraries,
                                  linkerType,
                                  linker,
                                  linkerArgs,
                                  flags,
                                  assemblyResources,
                                  importLibName,
                                  linkerStampExe,
                                  linkerStampExeArgs ) );
    AddNode( node );
    return node;
}

// CreateUnityNode
//------------------------------------------------------------------------------
UnityNode * NodeGraph::CreateUnityNode( const AString & unityName )
{
    ASSERT( Thread::IsMainThread() );

    UnityNode * node = FNEW( UnityNode() );
    node->SetName( unityName );
    AddNode( node );
    return node;
}

// CreateCSNode
//------------------------------------------------------------------------------
CSNode * NodeGraph::CreateCSNode( const AString & csAssemblyName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( csAssemblyName ) );

    CSNode * node = FNEW( CSNode() );
    node->SetName( csAssemblyName );
    AddNode( node );
    return node;
}

// CreateTestNode
//------------------------------------------------------------------------------
TestNode * NodeGraph::CreateTestNode( const AString & testOutput )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( testOutput ) );

    TestNode * node = FNEW( TestNode() );
    node->SetName( testOutput );
    AddNode( node );
    return node;
}

// CreateCompilerNode
//------------------------------------------------------------------------------
CompilerNode * NodeGraph::CreateCompilerNode( const AString & executable )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( executable ) );

    CompilerNode * node = FNEW( CompilerNode() );
    node->SetName( executable );
    AddNode( node );
    return node;
}

// CreateVCXProjectNode
//------------------------------------------------------------------------------
VCXProjectNode * NodeGraph::CreateVCXProjectNode( const AString & projectOutput,
                                                  const Array< AString > & projectBasePaths,
                                                  const Dependencies & paths,
                                                  const Array< AString > & pathsToExclude,
                                                  const Array< AString > & patternToExclude,
                                                  const Array< AString > & files,
                                                  const Array< AString > & filesToExclude,
                                                  const AString & rootNamespace,
                                                  const AString & projectGuid,
                                                  const AString & defaultLanguage,
                                                  const AString & applicationEnvironment,
                                                  const Array< VSProjectConfig > & configs,
                                                  const Array< VSProjectFileType > & fileTypes,
                                                  const Array< AString > & references,
                                                  const Array< AString > & projectReferences )
{
    ASSERT( Thread::IsMainThread() );

    AStackString< 1024 > fullPath;
    CleanPath( projectOutput, fullPath );

    VCXProjectNode * node = FNEW( VCXProjectNode( fullPath,
                                                projectBasePaths,
                                                paths,
                                                pathsToExclude,
                                                patternToExclude,
                                                files,
                                                filesToExclude,
                                                rootNamespace,
                                                projectGuid,
                                                defaultLanguage,
                                                applicationEnvironment,
                                                configs,
                                                fileTypes,
                                                references,
                                                projectReferences ) );
    AddNode( node );
    return node;
}

// CreateSLNNode
//------------------------------------------------------------------------------
SLNNode * NodeGraph::CreateSLNNode( const AString & solutionOutput,
                                    const AString & solutionBuildProject,
                                    const AString & solutionVisualStudioVersion,
                                    const AString & solutionMinimumVisualStudioVersion,
                                    const Array< VSProjectConfig > & configs,
                                    const Array< VCXProjectNode * > & projects,
                                    const Array< SLNDependency > & slnDeps,
                                    const Array< SLNSolutionFolder > & folders )
{
    ASSERT( Thread::IsMainThread() );

    AStackString< 1024 > fullPath;
    CleanPath( solutionOutput, fullPath );

    SLNNode * node = FNEW( SLNNode( fullPath,
                                    solutionBuildProject,
                                    solutionVisualStudioVersion,
                                    solutionMinimumVisualStudioVersion,
                                    configs,
                                    projects,
                                    slnDeps,
                                    folders ) );
    AddNode( node );
    return node;
}

// CreateObjectListNode
//------------------------------------------------------------------------------
ObjectListNode * NodeGraph::CreateObjectListNode( const AString & listName )
{
    ASSERT( Thread::IsMainThread() );

    ObjectListNode * node = FNEW( ObjectListNode() );
    node->SetName( listName );
    AddNode( node );
    return node;
}

// CreateXCodeProjectNode
//------------------------------------------------------------------------------
XCodeProjectNode * NodeGraph::CreateXCodeProjectNode( const AString & name )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( name ) );

    XCodeProjectNode * node = FNEW( XCodeProjectNode() );
    node->SetName( name );
    AddNode( node );
    return node;
}

// AddNode
//------------------------------------------------------------------------------
void NodeGraph::AddNode( Node * node )
{
    ASSERT( Thread::IsMainThread() );

    ASSERT( node );

    ASSERT( FindNodeInternal( node->GetName() ) == nullptr ); // node name must be unique

    // track in NodeMap
    const uint32_t crc = CRC32::CalcLower( node->GetName() );
    const size_t key = ( crc & 0xFFFF );
    node->m_Next = m_NodeMap[ key ];
    m_NodeMap[ key ] = node;

    // add to regular list
    if ( m_NextNodeIndex == m_AllNodes.GetSize() )
    {
        // normal addition of nodes to the end
        m_AllNodes.Append( node );
    }
    else
    {
        // inserting nodes during database load at specific indices
        ASSERT( m_AllNodes[ m_NextNodeIndex ] == nullptr );
        m_AllNodes[ m_NextNodeIndex ] = node;
    }

    // set index on node
    node->SetIndex( m_NextNodeIndex );
    m_NextNodeIndex = (uint32_t)m_AllNodes.GetSize();
}

// Build
//------------------------------------------------------------------------------
void NodeGraph::DoBuildPass( Node * nodeToBuild )
{
    PROFILE_FUNCTION

    s_BuildPassTag++;

    if ( nodeToBuild->GetType() == Node::PROXY_NODE )
    {
        const size_t total = nodeToBuild->GetStaticDependencies().GetSize();
        size_t failedCount = 0;
        size_t upToDateCount = 0;
        const Dependency * const end = nodeToBuild->GetStaticDependencies().End();
        for ( const Dependency * it = nodeToBuild->GetStaticDependencies().Begin(); it != end; ++it )
        {
            Node * n = it->GetNode();
            if ( n->GetState() == Node::FAILED )
            {
                failedCount++;
                continue;
            }
            else if ( n->GetState() == Node::UP_TO_DATE )
            {
                upToDateCount++;
                continue;
            }
            if ( n->GetState() != Node::BUILDING )
            {
                BuildRecurse( n, 0 );

                // check for nodes that become up-to-date immediately (trivial build)
                if ( n->GetState() == Node::UP_TO_DATE )
                {
                    upToDateCount++;
                }
            }
        }

        // only mark as failed or completed when all children have reached their final state
        if ( ( upToDateCount + failedCount ) == total )
        {
            // finished - mark with overall state
            nodeToBuild->SetState( failedCount ? Node::FAILED : Node::UP_TO_DATE );
        }
    }
    else
    {
        if ( nodeToBuild->GetState() < Node::BUILDING )
        {
            BuildRecurse( nodeToBuild, 0 );
        }
    }

    // Make available all the jobs we discovered in this pass
    JobQueue::Get().FlushJobBatch();
}

// BuildRecurse
//------------------------------------------------------------------------------
void NodeGraph::BuildRecurse( Node * nodeToBuild, uint32_t cost )
{
    ASSERT( nodeToBuild );

    // already building, or queued to build?
    ASSERT( nodeToBuild->GetState() != Node::BUILDING );

    // accumulate recursive cost
    cost += nodeToBuild->GetLastBuildTime();

    // check pre-build dependencies
    if ( nodeToBuild->GetState() == Node::NOT_PROCESSED )
    {
        // all static deps done?
        bool allDependenciesUpToDate = CheckDependencies( nodeToBuild, nodeToBuild->GetPreBuildDependencies(), cost );
        if ( allDependenciesUpToDate == false )
        {
            return; // not ready or failed
        }

        nodeToBuild->SetState( Node::PRE_DEPS_READY );
    }

    ASSERT( ( nodeToBuild->GetState() == Node::PRE_DEPS_READY ) ||
            ( nodeToBuild->GetState() == Node::STATIC_DEPS_READY ) ||
            ( nodeToBuild->GetState() == Node::DYNAMIC_DEPS_DONE ) );

    // test static dependencies first
    if ( nodeToBuild->GetState() == Node::PRE_DEPS_READY )
    {
        // all static deps done?
        bool allDependenciesUpToDate = CheckDependencies( nodeToBuild, nodeToBuild->GetStaticDependencies(), cost );
        if ( allDependenciesUpToDate == false )
        {
            return; // not ready or failed
        }

        nodeToBuild->SetState( Node::STATIC_DEPS_READY );
    }

    ASSERT( ( nodeToBuild->GetState() == Node::STATIC_DEPS_READY ) ||
            ( nodeToBuild->GetState() == Node::DYNAMIC_DEPS_DONE ) );

    if ( nodeToBuild->GetState() != Node::DYNAMIC_DEPS_DONE )
    {
        // static deps ready, update dynamic deps
        bool forceClean = FBuild::Get().GetOptions().m_ForceCleanBuild;
        if ( nodeToBuild->DoDynamicDependencies( *this, forceClean ) == false )
        {
            nodeToBuild->SetState( Node::FAILED );
            return;
        }

        nodeToBuild->SetState( Node::DYNAMIC_DEPS_DONE );
    }

    ASSERT( nodeToBuild->GetState() == Node::DYNAMIC_DEPS_DONE );

    // dynamic deps
    {
        // all static deps done?
        bool allDependenciesUpToDate = CheckDependencies( nodeToBuild, nodeToBuild->GetDynamicDependencies(), cost );
        if ( allDependenciesUpToDate == false )
        {
            return; // not ready or failed
        }
    }

    // dependencies are uptodate, so node can now tell us if it needs
    // building
    bool forceClean = FBuild::Get().GetOptions().m_ForceCleanBuild;
    nodeToBuild->SetStatFlag( Node::STATS_PROCESSED );
    if ( nodeToBuild->DetermineNeedToBuild( forceClean ) )
    {
        nodeToBuild->m_RecursiveCost = cost;
        JobQueue::Get().AddJobToBatch( nodeToBuild );
    }
    else
    {
        nodeToBuild->SetState( Node::UP_TO_DATE );
    }
}

// CheckDependencies
//------------------------------------------------------------------------------
bool NodeGraph::CheckDependencies( Node * nodeToBuild, const Dependencies & dependencies, uint32_t cost )
{
    ASSERT( nodeToBuild->GetType() != Node::PROXY_NODE );

    const uint32_t passTag = s_BuildPassTag;

    bool allDependenciesUpToDate = true;
    uint32_t numberNodesUpToDate = 0;
    uint32_t numberNodesFailed = 0;
    const bool stopOnFirstError = FBuild::Get().GetOptions().m_StopOnFirstError;

    Dependencies::Iter i = dependencies.Begin();
    Dependencies::Iter end = dependencies.End();
    for ( ; i < end; ++i )
    {
        Node * n = i->GetNode();

        Node::State state = n->GetState();

        // recurse into nodes which have not been processed yet
        if ( state < Node::BUILDING )
        {
            // early out if already seen
            if ( n->GetBuildPassTag() != passTag )
            {
                // prevent multiple recursions in this pass
                n->SetBuildPassTag( passTag );

                BuildRecurse( n, cost );
            }
        }

        // dependency is uptodate, nothing more to be done
        state = n->GetState();
        if ( state == Node::UP_TO_DATE )
        {
            ++numberNodesUpToDate;
            continue;
        }

        if ( state == Node::BUILDING )
        {
            // ensure deepest traversal cost is kept
            if ( cost > nodeToBuild->m_RecursiveCost )
            {
                nodeToBuild->m_RecursiveCost = cost;
            }
        }

        allDependenciesUpToDate = false;

        // dependency failed?
        if ( state == Node::FAILED )
        {
            ++numberNodesFailed;
            if ( stopOnFirstError )
            {
                // propogate failure state to this node
                nodeToBuild->SetState( Node::FAILED );
                break;
            }
        }

        // keep trying to progress other nodes...
    }

    if ( !stopOnFirstError )
    {
        if ( numberNodesFailed + numberNodesUpToDate == dependencies.GetSize() )
        {
            if ( numberNodesFailed > 0 )
            {
                nodeToBuild->SetState( Node::FAILED );
            }
        }
    }

    return allDependenciesUpToDate;
}

// CleanPath
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::CleanPath( AString & name )
{
    AStackString<> nameCopy( name );
    CleanPath( nameCopy, name );
}

// CleanPath
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::CleanPath( const AString & name, AString & fullPath )
{
    ASSERT( &name != &fullPath );

    char * dst;

    //  - path should be fully qualified
    bool isFullPath = PathUtils::IsFullPath( name );
    if ( !isFullPath )
    {
        // make a full path by prepending working dir
        const AString & workingDir = FBuild::Get().GetWorkingDir();

        // we're making the assumption that we don't need to clean the workingDir
        ASSERT( workingDir.Find( OTHER_SLASH ) == nullptr ); // bad slashes removed
        ASSERT( workingDir.Find( NATIVE_DOUBLE_SLASH ) == nullptr ); // redundant slashes removed

        // build the start of the path
        fullPath = workingDir;
        fullPath += NATIVE_SLASH;

        // concatenate
        uint32_t len = fullPath.GetLength();

        // make sure the dest will be big enough for the extra stuff
        fullPath.SetLength( fullPath.GetLength() + name.GetLength() );

        // set the output (which maybe a newly allocated ptr)
        dst = fullPath.Get() + len;
    }
    else
    {
        // make sure the dest will be big enough
        fullPath.SetLength( name.GetLength() );

        // copy from the start
        dst = fullPath.Get();
    }

    // the untrusted part of the path we need to copy/fix
    const char * src = name.Get();
    const char * const srcEnd = name.GetEnd();

    // clean slashes
    char lastChar = NATIVE_SLASH; // consider first item to follow a path (so "..\file.dat" works)
    #if defined( __WINDOWS__ )
        while ( *src == NATIVE_SLASH || *src == OTHER_SLASH ) { ++src; } // strip leading slashes
    #endif
    while ( src < srcEnd )
    {
        const char thisChar = *src;

        // hit a slash?
        if ( ( thisChar == NATIVE_SLASH ) || ( thisChar == OTHER_SLASH ) )
        {
            // write it the correct way
            *dst = NATIVE_SLASH;
            dst++;

            // skip until non-slashes
            while ( ( *src == NATIVE_SLASH ) || ( *src == OTHER_SLASH ) )
            {
                src++;
            }
            lastChar = NATIVE_SLASH;
            continue;
        }
        else if ( thisChar == '.' )
        {
            if ( lastChar == NATIVE_SLASH ) // fixed up slash, so we only need to check backslash
            {
                // check for \.\ (or \./)
                char nextChar = *( src + 1 );
                if ( ( nextChar == NATIVE_SLASH ) || ( nextChar == OTHER_SLASH ) )
                {
                    src++; // skip . and slashes
                    while ( ( *src == NATIVE_SLASH ) || ( *src == OTHER_SLASH ) )
                    {
                        ++src;
                    }
                    continue; // leave lastChar as-is, since we added nothing
                }

                // check for \..\ (or \../)
                if ( nextChar == '.' )
                {
                    nextChar = *( src + 2 );
                    if ( ( nextChar == NATIVE_SLASH ) || ( nextChar == OTHER_SLASH ) )
                    {
                        src+=2; // skip .. and slashes
                        while ( ( *src == NATIVE_SLASH ) || ( *src == OTHER_SLASH ) )
                        {
                            ++src;
                        }

                        if ( dst > fullPath.Get() + 3 )
                        {
                            --dst; // remove slash

                            // remove one level of path (but never past the absolute root)
                            #if defined( __WINDOWS__ )
                                while ( dst > fullPath.Get() + 3 ) // e.g. "c:\"
                            #else
                                while ( dst > fullPath.Get() + 1 ) // e.g. "/"
                            #endif
                            {
                                --dst;
                                if ( *dst == NATIVE_SLASH ) // only need to check for cleaned slashes
                                {
                                    ++dst; // keep this slash
                                    break;
                                }
                            }
                        }

                        continue;
                    }
                }
            }
        }

        // write non-slash character
        *dst++ = *src++;
        lastChar = thisChar;
    }

    // correct length of destination
    fullPath.SetLength( (uint16_t)( dst - fullPath.Get() ) );
    ASSERT( AString::StrLen( fullPath.Get() ) == fullPath.GetLength() );

    // sanity checks
    ASSERT( fullPath.Find( OTHER_SLASH ) == nullptr ); // bad slashes removed
    ASSERT( fullPath.Find( NATIVE_DOUBLE_SLASH ) == nullptr ); // redundant slashes removed
}

// AddUsedFile
//------------------------------------------------------------------------------
void NodeGraph::AddUsedFile( const AString & fileName, uint64_t timeStamp, uint64_t dataHash )
{
    const size_t numFiles = m_UsedFiles.GetSize();
    for ( size_t i=0 ;i<numFiles; ++i )
    {
        if ( PathUtils::ArePathsEqual( m_UsedFiles[i].m_FileName, fileName ) )
        {
            ASSERT( m_UsedFiles[ i ].m_Once == false ); // should not be trying to add a file a second time
            return;
        }
    }
    m_UsedFiles.Append( UsedFile( fileName, timeStamp, dataHash ) );
}

// IsOneUseFile
//------------------------------------------------------------------------------
bool NodeGraph::IsOneUseFile( const AString & fileName ) const
{
    const size_t numFiles = m_UsedFiles.GetSize();
    ASSERT( numFiles ); // shouldn't be called if there are no files
    for ( size_t i=0 ;i<numFiles; ++i )
    {
        if ( PathUtils::ArePathsEqual( m_UsedFiles[i].m_FileName, fileName ) )
        {
            return m_UsedFiles[ i ].m_Once;
        }
    }

    // file never seen, so it can be included multiple time initially
    // (if we hit a #once during parsing, we'll flag the file then)
    return false;
}

// SetCurrentFileAsOneUse
//------------------------------------------------------------------------------
void NodeGraph::SetCurrentFileAsOneUse()
{
    ASSERT( !m_UsedFiles.IsEmpty() );
    m_UsedFiles[ m_UsedFiles.GetSize() - 1 ].m_Once = true;
}

// FindNodeInternal
//------------------------------------------------------------------------------
Node * NodeGraph::FindNodeInternal( const AString & fullPath ) const
{
    ASSERT( Thread::IsMainThread() );

    const uint32_t crc = CRC32::CalcLower( fullPath );
    const size_t key = ( crc & 0xFFFF );

    Node * n = m_NodeMap[ key ];
    while ( n )
    {
        if ( n->GetNameCRC() == crc )
        {
            if ( n->GetName().CompareI( fullPath ) == 0 )
            {
                return n;
            }
        }
        n = n->m_Next;
    }
    return nullptr;
}

// FindNearestNodesInternal
//------------------------------------------------------------------------------
void NodeGraph::FindNearestNodesInternal( const AString & fullPath, Array< NodeWithDistance > & nodes, const uint32_t maxDistance ) const
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( nodes.IsEmpty() );
    ASSERT( false == nodes.IsAtCapacity() );

    if ( fullPath.IsEmpty() )
    {
        return;
    }

    uint32_t worstMinDistance = fullPath.GetLength() + 1;

    for ( size_t i = 0 ; i < NODEMAP_TABLE_SIZE ; i++ )
    {
        for ( Node * node = m_NodeMap[i] ; nullptr != node ; node = node->m_Next )
        {
            const uint32_t d = LevenshteinDistance::DistanceI( fullPath, node->GetName() );

            if ( d > maxDistance )
            {
                continue;
            }

            // skips nodes which don't share any character with fullpath
            if ( fullPath.GetLength() < node->GetName().GetLength() )
            {
                if ( d > node->GetName().GetLength() - fullPath.GetLength() )
                {
                    continue; // completly different <=> d deletions
                }
            }
            else
            {
                if ( d > fullPath.GetLength() - node->GetName().GetLength() )
                {
                    continue; // completly different <=> d deletions
                }
            }

            if ( nodes.IsEmpty() )
            {
                nodes.Append( NodeWithDistance( node, d ) );
                worstMinDistance = nodes.Top().m_Distance;
            }
            else if ( d >= worstMinDistance )
            {
                ASSERT( nodes.IsEmpty() || nodes.Top().m_Distance == worstMinDistance );
                if ( false == nodes.IsAtCapacity() )
                {
                    nodes.Append( NodeWithDistance( node, d ) );
                    worstMinDistance = d;
                }
            }
            else if ( d < worstMinDistance )
            {
                ASSERT( nodes.Top().m_Distance > d );
                const size_t count = nodes.GetSize();

                if ( false == nodes.IsAtCapacity() )
                {
                    nodes.Append(NodeWithDistance());
                }

                size_t pos = count;
                for ( ; pos > 0 ; pos-- )
                {
                    if ( nodes[pos - 1].m_Distance <= d )
                    {
                        break;
                    }
                    else if (pos < nodes.GetSize() )
                    {
                        nodes[pos] = nodes[pos - 1];
                    }
                }

                ASSERT( pos < count );
                nodes[pos] = NodeWithDistance( node, d );
                worstMinDistance = nodes.Top().m_Distance;
            }
        }
    }
}

// UpdateBuildStatus
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::UpdateBuildStatus( const Node * node,
                                              uint32_t & nodesBuiltTime,
                                              uint32_t & totalNodeTime )
{
    s_BuildPassTag++;
    UpdateBuildStatusRecurse( node, nodesBuiltTime, totalNodeTime );
}

// UpdateBuildStatusRecurse
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::UpdateBuildStatusRecurse( const Node * node,
                                                     uint32_t & nodesBuiltTime,
                                                     uint32_t & totalNodeTime )
{
    // record time for this node
    uint32_t nodeTime = node->GetLastBuildTime();
    totalNodeTime += nodeTime;
    nodesBuiltTime += ( node->GetState() == Node::UP_TO_DATE ) ? nodeTime : 0;

    // record accumulated child time if available
    uint32_t accumulatedProgress = node->GetProgressAccumulator();
    if ( accumulatedProgress > 0 )
    {
        nodesBuiltTime += accumulatedProgress;
        totalNodeTime += accumulatedProgress;
        return;
    }

    // don't recurse the same node multiple times in the same pass
    const uint32_t buildPassTag = s_BuildPassTag;
    if ( node->GetBuildPassTag() == buildPassTag )
    {
        return;
    }
    node->SetBuildPassTag( buildPassTag );

    // calculate time for children

    uint32_t progress = 0;
    uint32_t total = 0;

    UpdateBuildStatusRecurse( node->GetPreBuildDependencies(), progress, total );
    UpdateBuildStatusRecurse( node->GetStaticDependencies(), progress, total );
    UpdateBuildStatusRecurse( node->GetDynamicDependencies(), progress, total );

    nodesBuiltTime += progress;
    totalNodeTime += total;

    // if node is building, then progress of children cannot change
    // and we can store it in the accumulator
    if ( node->GetState() >= Node::BUILDING )
    {
        node->SetProgressAccumulator(total);
    }
}

// UpdateBuildStatusRecurse
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::UpdateBuildStatusRecurse( const Dependencies & dependencies,
                                                     uint32_t & nodesBuiltTime,
                                                     uint32_t & totalNodeTime )
{
    for ( Dependencies::Iter i = dependencies.Begin();
        i != dependencies.End();
        i++ )
    {
        UpdateBuildStatusRecurse( i->GetNode(), nodesBuiltTime, totalNodeTime );
    }
}

// ReadHeaderAndUsedFiles
//------------------------------------------------------------------------------
bool NodeGraph::ReadHeaderAndUsedFiles( IOStream & nodeGraphStream, const char* nodeGraphDBFile, Array< UsedFile > & files, bool & compatibleDB ) const
{
    // check for a valid header
    NodeGraphHeader ngh;
    if ( ( nodeGraphStream.Read( &ngh, sizeof( ngh ) ) != sizeof( ngh ) ) ||
         ( ngh.IsValid() == false ) )
    {
        return false;
    }

    // check if version is loadable
    if ( ngh.IsCompatibleVersion() == false )
    {
        compatibleDB = false;
        return true;
    }

    // Read location where .fdb was originally saved
    AStackString<> originalNodeGraphDBFile;
    if ( !nodeGraphStream.Read( originalNodeGraphDBFile ) )
    {
        return false;
    }
    AStackString<> nodeGraphDBFileClean( nodeGraphDBFile );
    NodeGraph::CleanPath( nodeGraphDBFileClean );
    if ( PathUtils::ArePathsEqual( originalNodeGraphDBFile, nodeGraphDBFileClean ) == false )
    {
        FLOG_WARN( "Database has been moved (originally at '%s', now at '%s').", originalNodeGraphDBFile.Get(), nodeGraphDBFileClean.Get() );
        return false;
    }

    uint32_t numFiles = 0;
    if ( !nodeGraphStream.Read( numFiles ) )
    {
        return false;
    }

    for ( uint32_t i=0; i<numFiles; ++i )
    {
        uint32_t fileNameLen( 0 );
        if ( !nodeGraphStream.Read( fileNameLen ) )
        {
            return false;
        }
        AStackString<> fileName;
        fileName.SetLength( fileNameLen ); // handles null terminate
        if ( nodeGraphStream.Read( fileName.Get(), fileNameLen ) != fileNameLen )
        {
            return false;
        }
        uint64_t timeStamp;
        if ( !nodeGraphStream.Read( timeStamp ) )
        {
            return false;
        }
        uint64_t dataHash;
        if ( !nodeGraphStream.Read( dataHash ) )
        {
            return false;
        }

        files.Append( UsedFile( fileName, timeStamp, dataHash ) );
    }

    return true;
}

// GetLibEnvVarHash
//------------------------------------------------------------------------------
uint32_t NodeGraph::GetLibEnvVarHash() const
{
    // ok for LIB var to be missing, we'll hash the empty string
    AStackString<> libVar;
    FBuild::Get().GetLibEnvVar( libVar );
    return xxHash::Calc32( libVar );
}

// IsCleanPath
//------------------------------------------------------------------------------
#if defined( ASSERTS_ENABLED )
    /*static*/ bool NodeGraph::IsCleanPath( const AString & path )
    {
        AStackString< 1024 > clean;
        CleanPath( path, clean );
        return ( path == clean );
    }
#endif

//------------------------------------------------------------------------------
