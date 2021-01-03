// Node.cpp - base interface for dependency graph nodes
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "NodeGraph.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/FunctionSettings.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_IgnoreForComparison.h"
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
#include "ListDependenciesNode.h"
#include "ObjectListNode.h"
#include "ObjectNode.h"
#include "RemoveDirNode.h"
#include "SettingsNode.h"
#include "SLNNode.h"
#include "TestNode.h"
#include "TextFileNode.h"
#include "UnityNode.h"
#include "VCXProjectNode.h"
#include "VSProjectExternalNode.h"
#include "XCodeProjectNode.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/Env/Env.h"
#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"
#include "Core/Math/xxHash.h"
#include "Core/Mem/Mem.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Reflection/ReflectedProperty.h"
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
, m_Settings( nullptr )
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
                                               const char * nodeGraphDBFile,
                                               bool forceMigration )
{
    PROFILE_FUNCTION;

    ASSERT( bffFile ); // must be supplied (or left as default)
    ASSERT( nodeGraphDBFile ); // must be supplied (or left as default)

    // Try to load the old DB
    NodeGraph * oldNG = FNEW( NodeGraph );
    LoadResult res = oldNG->Load( nodeGraphDBFile );

    // Tests can force us to do a migration even if the DB didn't change
    if ( forceMigration )
    {
        // If migration can't be forced, then the test won't function as expected
        // so we want to catch that failure.
        ASSERT( ( res == LoadResult::OK ) || ( res == LoadResult::OK_BFF_NEEDS_REPARSING ) );

        res = LoadResult::OK_BFF_NEEDS_REPARSING; // forces migration
    }

    // What happened?
    switch ( res )
    {
        case LoadResult::MISSING_OR_INCOMPATIBLE:
        case LoadResult::LOAD_ERROR:
        case LoadResult::LOAD_ERROR_MOVED:
        {
            // Failed due to moved DB?
            if ( res == LoadResult::LOAD_ERROR_MOVED )
            {
                // Is moving considerd fatal?
                if ( FBuild::Get().GetOptions().m_ContinueAfterDBMove == false )
                {
                    // Corrupt DB or other fatal problem
                    FDELETE( oldNG );
                    return nullptr;
                }
            }

            // Failed due to corrupt DB? Make a backup to assist triage
            if ( res == LoadResult::LOAD_ERROR )
            {
                AStackString<> corruptDBName( nodeGraphDBFile );
                corruptDBName += ".corrupt";
                FileIO::FileMove( AStackString<>( nodeGraphDBFile ), corruptDBName ); // Will overwrite if needed
            }
            
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
        case LoadResult::OK_BFF_NEEDS_REPARSING:
        {
            // Create a fresh DB by parsing the modified BFF
            NodeGraph * newNG = FNEW( NodeGraph );
            if ( newNG->ParseFromRoot( bffFile ) == false )
            {
                FDELETE( newNG );
                FDELETE( oldNG );
                return nullptr;
            }

            // Migrate old DB info to new DB
            const SettingsNode * settings = newNG->GetSettings();
            if ( ( settings->GetDisableDBMigration() == false ) || forceMigration )
            {
                newNG->Migrate( *oldNG );
            }
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

    // re-parse the BFF from scratch, clean build will result
    BFFParser bffParser( *this );
    const bool ok = bffParser.ParseFromFile( bffFile );
    if ( ok )
    {
        // Store a pointer to the SettingsNode as defined by the BFF, or create a
        // default instance if needed.
        const AStackString<> settingsNodeName( "$$Settings$$" );
        const Node * settingsNode = FindNode( settingsNodeName );
        m_Settings = settingsNode ? settingsNode->CastTo< SettingsNode >() : CreateSettingsNode( settingsNodeName ); // Create a default

        // Parser will populate m_UsedFiles
        const Array<BFFFile *> & usedFiles = bffParser.GetUsedFiles();
        m_UsedFiles.SetCapacity( usedFiles.GetSize() );
        for ( const BFFFile * file : usedFiles )
        {
            m_UsedFiles.EmplaceBack( file->GetFileName(), file->GetTimeStamp(), file->GetHash() );
        }
    }
    return ok;
}

// Load
//------------------------------------------------------------------------------
NodeGraph::LoadResult NodeGraph::Load( const char * nodeGraphDBFile )
{
    // Open previously saved DB
    FileStream fs;
    if ( fs.Open( nodeGraphDBFile, FileStream::READ_ONLY ) == false )
    {
        return LoadResult::MISSING_OR_INCOMPATIBLE;
    }

    // Read it into memory to avoid lots of tiny disk accesses
    const size_t fileSize = (size_t)fs.GetFileSize();
    UniquePtr< char > memory( (char *)ALLOC( fileSize ) );
    if ( fs.ReadBuffer( memory.Get(), fileSize ) != fileSize )
    {
        FLOG_ERROR( "Could not read Database. Error: %s File: '%s'", LAST_ERROR_STR, nodeGraphDBFile );
        return LoadResult::LOAD_ERROR;
    }
    ConstMemoryStream ms( memory.Get(), fileSize );

    // Load the Old DB
    NodeGraph::LoadResult res = Load( ms, nodeGraphDBFile );
    if ( res == LoadResult::LOAD_ERROR )
    {
        FLOG_ERROR( "Database corrupt (clean build will occur): '%s'", nodeGraphDBFile );
    }
    return res;
}

// Load
//------------------------------------------------------------------------------
NodeGraph::LoadResult NodeGraph::Load( IOStream & stream, const char * nodeGraphDBFile )
{
    bool compatibleDB;
    bool movedDB;
    Array< UsedFile > usedFiles;
    if ( ReadHeaderAndUsedFiles( stream, nodeGraphDBFile, usedFiles, compatibleDB, movedDB ) == false )
    {
        return movedDB ? LoadResult::LOAD_ERROR_MOVED : LoadResult::LOAD_ERROR;
    }

    // old or otherwise incompatible DB version?
    if ( !compatibleDB )
    {
        FLOG_WARN( "Database version has changed (clean build will occur)." );
        return LoadResult::MISSING_OR_INCOMPATIBLE;
    }

    // Take not of whether we need to reparse
    bool bffNeedsReparsing = false;

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
            if ( !bffNeedsReparsing )
            {
                FLOG_VERBOSE( "BFF file '%s' missing or unopenable (reparsing will occur).", fileName.Get() );
                bffNeedsReparsing = true;
            }
            continue; // not opening the file is not an error, it could be not needed anymore
        }

        const size_t size = (size_t)fs.GetFileSize();
        UniquePtr< void > mem( ALLOC( size ) );
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

        // Tell used reparsing will occur (Warn only about the first file)
        if ( !bffNeedsReparsing )
        {
            FLOG_WARN( "BFF file '%s' has changed (reparsing will occur).", fileName.Get() );
            bffNeedsReparsing = true;
        }
    }

    m_UsedFiles = usedFiles;

    // TODO:C The serialization of these settings doesn't really belong here (not part of node graph)

    // environment
    uint32_t envStringSize = 0;
    if ( stream.Read( envStringSize ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }
    UniquePtr< char > envString;
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
                // make sure the user knows why some things might re-build (only the first thing warns)
                if ( !bffNeedsReparsing )
                {
                    FLOG_WARN( "'%s' Environment variable was not found - BFF will be re-parsed\n", varName.Get() );
                    bffNeedsReparsing = true;
                }
            }
            if ( importedVarHash != savedVarHash )
            {
                // make sure the user knows why some things might re-build (only the first thing warns)
                if ( !bffNeedsReparsing )
                {
                    FLOG_WARN( "'%s' Environment variable has changed - BFF will be re-parsed\n", varName.Get() );
                    bffNeedsReparsing = true;
                }
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
            // make sure the user knows why some things might re-build (only the first thing warns)
            if ( !bffNeedsReparsing )
            {
                FLOG_WARN( "'%s' Environment variable has changed - BFF will be re-parsed\n", "LIB" );
                bffNeedsReparsing = true;
            }
        }
    }

    // Files use in file_exists checks
    BFFFileExists fileExistsInfo;
    if ( fileExistsInfo.Load( stream ) == false )
    {
        return LoadResult::LOAD_ERROR;
    }
    bool added;
    const AString * changedFile = fileExistsInfo.CheckForChanges( added );
    if ( changedFile )
    {
        FLOG_WARN( "File used in file_exists was %s '%s' - BFF will be re-parsed\n", added ? "added" : "removed", changedFile->Get() );
        bffNeedsReparsing = true;
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

    m_Settings = FindNode( AStackString<>( "$$Settings$$" ) )->CastTo< SettingsNode >();
    ASSERT( m_Settings );

    if ( bffNeedsReparsing )
    {
        return LoadResult::OK_BFF_NEEDS_REPARSING;
    }

    // Everything OK - propagate global settings
    //------------------------------------------------

    // file_exists files
    FBuild::Get().GetFileExistsInfo() = fileExistsInfo;

    // Environment
    if ( envStringSize > 0 )
    {
        FBuild::Get().SetEnvironmentString( envString.Get(), envStringSize, libEnvVar );
    }

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
    const Node * const n = Node::Load( *this, stream );
    if ( n == nullptr )
    {
        return false;
    }

    // sanity check node index was correctly restored
    ASSERT( m_AllNodes[ nodeIndex ] == n );
    ASSERT( n->GetIndex() == nodeIndex );

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
        stream.Write( fileName );
        uint64_t timeStamp( m_UsedFiles[ i ].m_TimeStamp );
        stream.Write( timeStamp );
        uint64_t dataHash( m_UsedFiles[ i ].m_DataHash );
        stream.Write( dataHash );
    }

    // TODO:C The serialization of these settings doesn't really belong here (not part of node graph)
    {
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
    }

    // Write file_exists tracking info
    FBuild::Get().GetFileExistsInfo().Save( stream );

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

// SerializeToText
//------------------------------------------------------------------------------
void NodeGraph::SerializeToText( const Dependencies & deps, AString & outBuffer ) const
{
    s_BuildPassTag++;

    if ( deps.IsEmpty() == false )
    {
        for ( const Dependency & dep : deps )
        {
            SerializeToText( dep.GetNode(), 0, outBuffer );
        }
    }
    else
    {
        for ( Node * node : m_AllNodes )
        {
            SerializeToText( node, 0, outBuffer );
        }
    }
}

// SerializeToText
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::SerializeToText( Node * node, uint32_t depth, AString& outBuffer )
{
    // Print this even if it has been visited before so the edge is visible
    outBuffer.AppendFormat( "%*s%s %s\n", depth * 4, "", node->GetTypeName(), node->GetName().Get() );

    // Don't descend into already visited nodes
    if ( node->GetBuildPassTag() == s_BuildPassTag )
    {
        if ( node->GetPreBuildDependencies().GetSize() ||
             node->GetStaticDependencies().GetSize() ||
             node->GetDynamicDependencies().GetSize() )
        {
            outBuffer.AppendFormat( "%*s...\n", ( depth + 1 ) * 4, "" );
        }
        return;
    }
    node->SetBuildPassTag( s_BuildPassTag );

    // Dependencies
    SerializeToText( "PreBuild", node->GetPreBuildDependencies(), depth, outBuffer );
    SerializeToText( "Static", node->GetStaticDependencies(), depth, outBuffer );
    SerializeToText( "Dynamic", node->GetDynamicDependencies(), depth, outBuffer );
}

// SerializeToText
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::SerializeToText( const char * title, const Dependencies & dependencies, uint32_t depth, AString & outBuffer )
{
    const Dependency * const end = dependencies.End();
    const Dependency * it = dependencies.Begin();
    if ( it != end )
    {
        outBuffer.AppendFormat( "%*s%s\n", depth * 4 + 2, "", title );
    }
    for ( ; it != end; ++it )
    {
        Node * n = it->GetNode();
        SerializeToText( n, depth + 1, outBuffer );
    }
}

// SerializeToDotFormat
//------------------------------------------------------------------------------
void NodeGraph::SerializeToDotFormat( const Dependencies & deps,
                                      const bool fullGraph,
                                      AString & outBuffer ) const
{
    s_BuildPassTag++; // Used to mark nodes as we sweep to visit each node only once

    // Header of DOT format
    outBuffer += "digraph G\n";
    outBuffer += "{\n";
    outBuffer += "\trankdir=LR\n";
    outBuffer += "\tnode [shape=record;style=filled]\n";

    if ( deps.IsEmpty() == false )
    {
        // Emit subset of graph for specified targets
        for ( const Dependency & dep : deps )
        {
            SerializeToDot( dep.GetNode(), fullGraph, outBuffer );
        }
    }
    else
    {
        // Emit entire graph
        for ( Node * node : m_AllNodes )
        {
            SerializeToDot( node, fullGraph, outBuffer );
        }
    }

    // Footer of DOT format
    outBuffer += "}\n";
}

// SerializeToDot
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::SerializeToDot( Node * node,
                                                 const bool fullGraph,
                                                 AString & outBuffer )
{
    // Early out for nodes we've already visited
    if ( node->GetBuildPassTag() == s_BuildPassTag )
    {
        return;
    }
    node->SetBuildPassTag( s_BuildPassTag ); // Mark as visited

    // If outputting a reduced graph, prune out any leaf FileNodes.
    // (i.e. files that exist outside of the build - typically source code files)
    const bool isLeafFileNode = ( node->GetType() == Node::FILE_NODE );
    if ( isLeafFileNode && ( fullGraph == false ) )
    {
        return; // Strip this node
    }

    // Name of this node
    AStackString<> name( node->GetName() );
    name.Replace( "\\", "\\\\" ); // Escape slashes in this name
    outBuffer.AppendFormat( "\n\t\"%s\" %s // %s\n",
                            name.Get(),
                            isLeafFileNode ? "[style=none]" : "",
                            node->GetTypeName() );

    // Dependencies
    SerializeToDot( "PreBuild", "[style=dashed]", node, node->GetPreBuildDependencies(), fullGraph, outBuffer );
    SerializeToDot( "Static", nullptr, node, node->GetStaticDependencies(), fullGraph, outBuffer );
    SerializeToDot( "Dynamic", "[color=gray]", node, node->GetDynamicDependencies(), fullGraph, outBuffer );

    // Recurse into Dependencies
    SerializeToDot( node->GetPreBuildDependencies(), fullGraph, outBuffer );
    SerializeToDot( node->GetStaticDependencies(), fullGraph, outBuffer );
    SerializeToDot( node->GetDynamicDependencies(), fullGraph, outBuffer );
}

// SerializeToDot
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::SerializeToDot( const char * dependencyType,
                                                 const char * style,
                                                 const Node * node,
                                                 const Dependencies & dependencies,
                                                 const bool fullGraph,
                                                 AString & outBuffer )
{
    if ( dependencies.IsEmpty() )
    {
        return;
    }

    // Escape slashes in this name
    AStackString<> left( node->GetName() );
    left.Replace( "\\", "\\\\" );

    // All the dependencies
    for ( const Dependency & dep : dependencies )
    {
        // If outputting a reduced graph, prune out links to leaf FileNodes.
        // (i.e. files that exist outside of the build - typically source code files)
        if ( ( fullGraph == false ) && ( dep.GetNode()->GetType() == Node::FILE_NODE ) )
        {
            continue;
        }

        // Write the graph edge
        AStackString<> right( dep.GetNode()->GetName() );
        right.Replace( "\\", "\\\\" );
        outBuffer.AppendFormat( "\t\t/*%-8s*/ \"%s\" -> \"%s\"",
                                dependencyType,
                                left.Get(),
                                right.Get() );

        // Append the optional style
        if ( style )
        {
            outBuffer += ' ';
            outBuffer += style;
        }

        // Temrinate the line
        outBuffer += '\n';
    }
}

// SerializeToDot
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::SerializeToDot( const Dependencies & dependencies,
                                                 const bool fullGraph,
                                                 AString & outBuffer )
{
    for ( const Dependency & dep : dependencies )
    {
        SerializeToDot( dep.GetNode(), fullGraph, outBuffer );
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

// FindNodeExact (AString &)
//------------------------------------------------------------------------------
Node * NodeGraph::FindNodeExact( const AString & nodeName ) const
{
    // try to find node 'as is'
    return FindNodeInternal( nodeName );
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

// RegisterNode
//------------------------------------------------------------------------------
void NodeGraph::RegisterNode( Node * node )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( node->GetName().IsEmpty() == false );
    ASSERT( FindNode( node->GetName() ) == nullptr );
    AddNode( node );
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
RemoveDirNode * NodeGraph::CreateRemoveDirNode( const AString & nodeName )
{
    ASSERT( Thread::IsMainThread() );

    RemoveDirNode * node = FNEW( RemoveDirNode() );
    node->SetName( nodeName );
    AddNode( node );
    return node;
}

// CreateExecNode
//------------------------------------------------------------------------------
ExecNode * NodeGraph::CreateExecNode( const AString & nodeName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( nodeName ) );

    ExecNode * node = FNEW( ExecNode() );
    node->SetName( nodeName );
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
        node = FNEW( FileNode( fullPath, Node::FLAG_ALWAYS_BUILD ) );
    }
    else
    {
        node = FNEW( FileNode( fileName, Node::FLAG_ALWAYS_BUILD ) );
    }

    AddNode( node );
    return node;
}

// CreateDirectoryListNode
//------------------------------------------------------------------------------
DirectoryListNode * NodeGraph::CreateDirectoryListNode( const AString & name )
{
    ASSERT( Thread::IsMainThread() );

    DirectoryListNode * node = FNEW( DirectoryListNode() );
    node->SetName( name );
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
DLLNode * NodeGraph::CreateDLLNode( const AString & dllName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( dllName ) );

    DLLNode * node = FNEW( DLLNode() );
    node->SetName( dllName );
    AddNode( node );
    return node;
}

// CreateExeNode
//------------------------------------------------------------------------------
ExeNode * NodeGraph::CreateExeNode( const AString & exeName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( exeName ) );

    ExeNode * node = FNEW( ExeNode() );
    node->SetName( exeName );
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
CompilerNode * NodeGraph::CreateCompilerNode( const AString & name )
{
    ASSERT( Thread::IsMainThread() );

    CompilerNode * node = FNEW( CompilerNode() );
    node->SetName( name );
    AddNode( node );
    return node;
}

// CreateVCXProjectNode
//------------------------------------------------------------------------------
VSProjectBaseNode * NodeGraph::CreateVCXProjectNode( const AString & name )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( name ) );

    VCXProjectNode * node = FNEW( VCXProjectNode() );
    node->SetName( name );
    AddNode( node );
    return node;
}

// CreateVSProjectExternalNode
//------------------------------------------------------------------------------
VSProjectBaseNode * NodeGraph::CreateVSProjectExternalNode(const AString& name)
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( name ) );

    VSProjectExternalNode* node = FNEW( VSProjectExternalNode() );
    node->SetName( name );
    AddNode( node );
    return node;
}

// CreateSLNNode
//------------------------------------------------------------------------------
SLNNode * NodeGraph::CreateSLNNode( const AString & name )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( name ) );

    SLNNode * node = FNEW( SLNNode() );
    node->SetName( name );
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

// CreateSettingsNode
//------------------------------------------------------------------------------
SettingsNode * NodeGraph::CreateSettingsNode( const AString & name )
{
    ASSERT( Thread::IsMainThread() );

    SettingsNode * node = FNEW( SettingsNode() );
    node->SetName( name );
    AddNode( node );
    return node;
}

// CreateListDependenciesNode
//------------------------------------------------------------------------------
ListDependenciesNode * NodeGraph::CreateListDependenciesNode( const AString & name )
{
    ASSERT( Thread::IsMainThread() );

    ListDependenciesNode * node = FNEW( ListDependenciesNode() );
    node->SetName( name );
    AddNode( node );
    return node;
}

// CreateTextFileNode
//------------------------------------------------------------------------------
TextFileNode* NodeGraph::CreateTextFileNode( const AString& nodeName )
{
    ASSERT( Thread::IsMainThread() );
    ASSERT( IsCleanPath( nodeName ) );

    TextFileNode* node = FNEW( TextFileNode() );
    node->SetName( nodeName );
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
    PROFILE_FUNCTION;

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
            if ( n->GetState() < Node::BUILDING )
            {
                BuildRecurse( n, 0 );
            }

            // check result of recursion (which may or may not be complete)
            if ( n->GetState() == Node::UP_TO_DATE )
            {
                upToDateCount++;
            }
            else if ( n->GetState() == Node::FAILED )
            {
                failedCount++;
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
        // all pre-build deps done?
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
        // If static deps require us to rebuild, dynamic dependencies need regenerating
        const bool forceClean = FBuild::Get().GetOptions().m_ForceCleanBuild;
        if ( forceClean ||
             nodeToBuild->DetermineNeedToBuild( nodeToBuild->GetStaticDependencies() ) )
        {
            // Clear dynamic dependencies
            nodeToBuild->m_DynamicDependencies.Clear();

            // Explicitly mark node in a way that will result in it rebuilding should
            // we cancel the build before builing this node
            if ( nodeToBuild->m_Stamp == 0 )
            {
                // Note that this is the first time we're building (since Node can't check
                // stamp as we clear it below)
                nodeToBuild->SetStatFlag( Node::STATS_FIRST_BUILD );
            }
            nodeToBuild->m_Stamp = 0;

            // Regenerate dynamic dependencies
            if ( nodeToBuild->DoDynamicDependencies( *this, forceClean ) == false )
            {
                nodeToBuild->SetState( Node::FAILED );
                return;
            }

            // Continue through to check dynamic dependencies and build
        }

        // Dynamic dependencies are ready to be checked
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
    if ( forceClean ||
         ( nodeToBuild->GetStamp() == 0 ) || // Avoid redundant messages from DetermineNeedToBuild
         nodeToBuild->DetermineNeedToBuild( nodeToBuild->GetStaticDependencies() ) ||
         nodeToBuild->DetermineNeedToBuild( nodeToBuild->GetDynamicDependencies() ) )
    {
        nodeToBuild->m_RecursiveCost = cost;
        JobQueue::Get().AddJobToBatch( nodeToBuild );
    }
    else
    {
        if ( FLog::ShowVerbose() )
        {
            FLOG_BUILD_REASON( "Up-To-Date '%s'\n", nodeToBuild->GetName().Get() );
        }
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
/*static*/ void NodeGraph::CleanPath( AString & name, bool makeFullPath )
{
    AStackString<> nameCopy( name );
    CleanPath( nameCopy, name, makeFullPath );
}

// CleanPath
//------------------------------------------------------------------------------
/*static*/ void NodeGraph::CleanPath( const AString & name, AString & cleanPath, bool makeFullPath )
{
    ASSERT( &name != &cleanPath );

    char * dst;

    //  - path can be fully qualified
    bool isFullPath = PathUtils::IsFullPath( name );
    if ( !isFullPath && makeFullPath )
    {
        // make a full path by prepending working dir
        const AString & workingDir = FBuild::IsValid() ? FBuild::Get().GetWorkingDir() : AString::GetEmpty();

        // we're making the assumption that we don't need to clean the workingDir
        ASSERT( workingDir.Find( OTHER_SLASH ) == nullptr ); // bad slashes removed
        ASSERT( workingDir.Find( NATIVE_DOUBLE_SLASH ) == nullptr ); // redundant slashes removed

        // build the start of the path
        cleanPath = workingDir;
        cleanPath += NATIVE_SLASH;

        // concatenate
        uint32_t len = cleanPath.GetLength();

        // make sure the dest will be big enough for the extra stuff
        cleanPath.SetLength( cleanPath.GetLength() + name.GetLength() );

        // set the output (which maybe a newly allocated ptr)
        dst = cleanPath.Get() + len;

        isFullPath = true;
    }
    else
    {
        // make sure the dest will be big enough
        cleanPath.SetLength( name.GetLength() );

        // copy from the start
        dst = cleanPath.Get();
    }

    // the untrusted part of the path we need to copy/fix
    const char * src = name.Get();
    const char * const srcEnd = name.GetEnd();

    // clean slashes
    char lastChar = NATIVE_SLASH; // consider first item to follow a path (so "..\file.dat" works)
    #if defined( __WINDOWS__ )
        while ( *src == NATIVE_SLASH || *src == OTHER_SLASH ) { ++src; } // strip leading slashes
    #endif

    const char * lowestRemovableChar = cleanPath.Get();
    if ( isFullPath )
    {
        #if defined( __WINDOWS__ )
            lowestRemovableChar += 3; // e.g. "c:\"
        #else
            lowestRemovableChar += 1; // e.g. "/"
        #endif
    }

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
                    if ( ( nextChar == NATIVE_SLASH ) || ( nextChar == OTHER_SLASH ) || ( nextChar == '\0' ) )
                    {
                        src+=2; // skip .. and slashes
                        while ( ( *src == NATIVE_SLASH ) || ( *src == OTHER_SLASH ) )
                        {
                            ++src;
                        }

                        if ( dst > lowestRemovableChar )
                        {
                            --dst; // remove slash

                            while ( dst > lowestRemovableChar ) // e.g. "c:\"
                            {
                                --dst;
                                if ( *dst == NATIVE_SLASH ) // only need to check for cleaned slashes
                                {
                                    ++dst; // keep this slash
                                    break;
                                }
                            }
                        }
                        else if ( !isFullPath )
                        {
                            *dst++ = '.';
                            *dst++ = '.';
                            *dst++ = NATIVE_SLASH;
                            lowestRemovableChar = dst;
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
    cleanPath.SetLength( (uint16_t)( dst - cleanPath.Get() ) );
    ASSERT( AString::StrLen( cleanPath.Get() ) == cleanPath.GetLength() );

    // sanity checks
    ASSERT( cleanPath.Find( OTHER_SLASH ) == nullptr ); // bad slashes removed
    ASSERT( cleanPath.Find( NATIVE_DOUBLE_SLASH ) == nullptr ); // redundant slashes removed
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
                nodes.EmplaceBack( node, d );
                worstMinDistance = nodes.Top().m_Distance;
            }
            else if ( d >= worstMinDistance )
            {
                ASSERT( nodes.IsEmpty() || nodes.Top().m_Distance == worstMinDistance );
                if ( false == nodes.IsAtCapacity() )
                {
                    nodes.EmplaceBack( node, d );
                    worstMinDistance = d;
                }
            }
            else
            {
                ASSERT( nodes.Top().m_Distance > d );
                const size_t count = nodes.GetSize();

                if ( false == nodes.IsAtCapacity() )
                {
                    nodes.EmplaceBack();
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
bool NodeGraph::ReadHeaderAndUsedFiles( IOStream & nodeGraphStream, const char* nodeGraphDBFile, Array< UsedFile > & files, bool & compatibleDB, bool & movedDB ) const
{
    // Assume good DB by default (cases below will change flags if needed)
    compatibleDB = true;
    movedDB = false;

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
        movedDB = true;
        FLOG_WARN( "Database has been moved (originally at '%s', now at '%s').", originalNodeGraphDBFile.Get(), nodeGraphDBFileClean.Get() );
        if ( FBuild::Get().GetOptions().m_ContinueAfterDBMove )
        {
            // Allow build to continue (will be a clean build)
            compatibleDB = false;
            return true;
        }
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

        files.EmplaceBack( fileName, timeStamp, dataHash );
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

// Migrate
//------------------------------------------------------------------------------
void NodeGraph::Migrate( const NodeGraph & oldNodeGraph )
{
    PROFILE_FUNCTION;

    s_BuildPassTag++;

    // NOTE: m_AllNodes can change during recursion, so we must take care to
    // iterate by index (array might move due to resizing). Any newly added
    // nodes will already be traversed so we only need to check the original
    // range here
    const size_t numNodes = m_AllNodes.GetSize();
    for ( size_t i=0; i<numNodes; ++i )
    {
        Node & newNode = *m_AllNodes[ i ];
        MigrateNode( oldNodeGraph, newNode, nullptr );
    }
}

// MigrateNode
//------------------------------------------------------------------------------
void NodeGraph::MigrateNode( const NodeGraph & oldNodeGraph, Node & newNode, const Node * oldNodeHint )
{
    // Prevent visiting the same node twice
    if ( newNode.GetBuildPassTag() == s_BuildPassTag )
    {
        return;
    }
    newNode.SetBuildPassTag( s_BuildPassTag );

    // FileNodes (inputs to the build) build every time so don't need migration
    if ( newNode.GetType() == Node::FILE_NODE )
    {
        return;
    }

    // Migrate children before parents
    for ( Dependency & dep : newNode.m_PreBuildDependencies )
    {
        MigrateNode( oldNodeGraph, *dep.GetNode(), nullptr );
    }
    for ( Dependency& dep : newNode.m_StaticDependencies )
    {
        MigrateNode( oldNodeGraph, *dep.GetNode(), nullptr );
    }

    // Get the matching node in the old DB
    const Node * oldNode;
    if ( oldNodeHint )
    {
        // Use the node passed in (if calling code already knows it saves us a lookup)
        oldNode = oldNodeHint;
        ASSERT( oldNode == oldNodeGraph.FindNodeInternal( newNode.GetName() ) );
    }
    else
    {
        // Find the old node
        oldNode = oldNodeGraph.FindNodeInternal( newNode.GetName() );
        if ( oldNode == nullptr )
        {
            // The newNode has no equivalent in the old DB.
            // This is either:
            //  - a brand new target defined in the bff
            //  - an existing target that has never been built
            return;
        }
    }

    // Has the node changed type?
    const ReflectionInfo * oldNodeRI = oldNode->GetReflectionInfoV();
    const ReflectionInfo * newNodeRI = newNode.GetReflectionInfoV();
    if ( oldNodeRI != newNodeRI )
    {
        // The newNode has changed type (the build rule has changed)
        return;
    }

    // Have the properties on the node changed?
    if ( AreNodesTheSame( oldNode, &newNode, newNodeRI ) == false )
    {
        // Properties have changed. We need to rebuild with the new
        // properties.
        return;
    }

    // PreBuildDependencies
    if ( DoDependenciesMatch( oldNode->m_PreBuildDependencies, newNode.m_PreBuildDependencies ) == false )
    {
        return;
    }

    // StaticDependencies
    if ( DoDependenciesMatch( oldNode->m_StaticDependencies, newNode.m_StaticDependencies ) == false )
    {
        return;
    }

    // Migrate static Dependencies
    // - since everything matches, we only need to migrate the stamps
    for ( Dependency & dep : newNode.m_StaticDependencies )
    {
        const size_t index = size_t( &dep - newNode.m_StaticDependencies.Begin() );
        const Dependency & oldDep = oldNode->m_StaticDependencies[ index ];
        dep.Stamp( oldDep.GetNodeStamp() );
    }

    // Migrate Dynamic Dependencies
    {
        // New node should have no dynamic dependencies
        ASSERT( newNode.m_DynamicDependencies.GetSize() == 0 );
        const Array< Dependency > & oldDeps = oldNode->m_DynamicDependencies;
        Array< Dependency > newDeps( oldDeps.GetSize() );
        for ( const Dependency & oldDep : oldDeps )
        {
            // See if the depenceny already exists in the new DB
            const Node * oldDepNode = oldDep.GetNode();
            Node * newDepNode = FindNodeInternal( oldDepNode->GetName() );

            // If the dependency exists, but has changed type, then dependencies
            // cannot be transferred.
            if ( newDepNode && ( newDepNode->GetType() != oldDepNode->GetType() ) )
            {
                return; // No point trying the remaining deps as node will need rebuilding anyway
            }
            if ( newDepNode )
            {
                newDeps.EmplaceBack( newDepNode, oldDep.GetNodeStamp(), oldDep.IsWeak() );
            }
            else
            {
                // Create the dependency
                newDepNode = Node::CreateNode( *this, oldDepNode->GetType(), oldDepNode->GetName() );
                ASSERT( newDepNode );
                newDeps.EmplaceBack( newDepNode, oldDep.GetNodeStamp(), oldDep.IsWeak() );

                // Early out for FileNode (no properties and doesn't need Initialization)
                if ( oldDepNode->GetType() == Node::FILE_NODE )
                {
                    continue;
                }

                // Transfer all the properties
                MigrateProperties( (const void *)oldDepNode, (void *)newDepNode, newDepNode->GetReflectionInfoV() );

                // Initialize the new node
                const BFFToken * token = nullptr;
                VERIFY( newDepNode->Initialize( *this, token, nullptr ) );

                // Continue recursing
                MigrateNode( oldNodeGraph, *newDepNode, oldDepNode );
            }
        }
        if ( newDeps.IsEmpty() == false )
        {
            newNode.m_DynamicDependencies.Append( newDeps );
        }
    }

    // If we get here, then everything about the node is unchanged from the
    // old DB to the new DB, so we can transfer the node's internal state. This
    // will prevent the node rebuilding unnecessarily.
    newNode.Migrate( *oldNode );
}

// MigrateProperties
//------------------------------------------------------------------------------
void NodeGraph::MigrateProperties( const void * oldBase, void * newBase, const ReflectionInfo * ri )
{
    // Are all properties the same?
    do
    {
        const ReflectionIter end = ri->End();
        for ( ReflectionIter it = ri->Begin(); it != end; ++it )
        {
            MigrateProperty( oldBase, newBase, *it );
        }

        // Traverse into parent class (if there is one)
        ri = ri->GetSuperClass();
    }
    while( ri );
}

// MigrateProperty
//------------------------------------------------------------------------------
void NodeGraph::MigrateProperty( const void * oldBase, void * newBase, const ReflectedProperty & property )
{
    switch ( property.GetType() )
    {
        case PropertyType::PT_ASTRING:
        {
            if ( property.IsArray() )
            {
                const Array< AString > * stringsOld = property.GetPtrToArray<AString>( oldBase );
                Array< AString > * stringsNew = property.GetPtrToArray<AString>( newBase );
                *stringsNew = *stringsOld;
            }
            else
            {
                const AString * stringOld = property.GetPtrToProperty<AString>( oldBase );
                AString * stringNew = property.GetPtrToProperty<AString>( newBase );
                *stringNew = *stringOld;
            }
            break;
        }
        case PT_UINT8:
        {
            ASSERT( property.IsArray() == false );

            const uint8_t * u8Old = property.GetPtrToProperty<uint8_t>( oldBase );
            uint8_t * u8New = property.GetPtrToProperty<uint8_t>( newBase );
            *u8New = *u8Old;
            break;
        }
        case PT_INT32:
        {
            ASSERT( property.IsArray() == false );
            const int32_t * i32Old = property.GetPtrToProperty<int32_t>( oldBase );
            int32_t * i32New = property.GetPtrToProperty<int32_t>( newBase );
            *i32New = *i32Old;
            break;
        }
        case PT_UINT32:
        {
            ASSERT( property.IsArray() == false );
            const uint32_t * u32Old = property.GetPtrToProperty<uint32_t>( oldBase );
            uint32_t * u32New = property.GetPtrToProperty<uint32_t>( newBase );
            *u32New = *u32Old;
            break;
        }
        case PT_UINT64:
        {
            ASSERT( property.IsArray() == false );
            const uint64_t * u64Old = property.GetPtrToProperty<uint64_t>( oldBase );
            uint64_t * u64New = property.GetPtrToProperty<uint64_t>( newBase );
            *u64New = *u64Old;
            break;
        }
        case PT_BOOL:
        {
            ASSERT( property.IsArray() == false );
            const bool * bOld = property.GetPtrToProperty<bool>( oldBase );
            bool * bNew = property.GetPtrToProperty<bool>( newBase );
            *bNew = *bOld;
            break;
        }
        case PT_STRUCT:
        {
            const ReflectedPropertyStruct & propertyStruct = static_cast<const ReflectedPropertyStruct &>( property );
            if ( property.IsArray() )
            {
                const uint32_t numElements = (uint32_t)propertyStruct.GetArraySize( oldBase );
                propertyStruct.ResizeArrayOfStruct( newBase, numElements );
                for ( uint32_t i=0; i<numElements; ++i )
                {
                    MigrateProperties( propertyStruct.GetStructInArray( oldBase, i ), propertyStruct.GetStructInArray( newBase, i ), propertyStruct.GetStructReflectionInfo() );
                }
            }
            else
            {
                MigrateProperties( propertyStruct.GetStructBase( oldBase ), propertyStruct.GetStructBase( newBase ), propertyStruct.GetStructReflectionInfo() );
            }
            break;
        }
        default: ASSERT( false ); // Unhandled
    }
}

// AreNodesTheSame
//------------------------------------------------------------------------------
/*static*/ bool NodeGraph::AreNodesTheSame( const void * baseA, const void * baseB, const ReflectionInfo * ri )
{
    // Are all properties the same?
    do
    {
        const ReflectionIter end = ri->End();
        for ( ReflectionIter it = ri->Begin(); it != end; ++it )
        {
            // Is this property the same?
            if ( AreNodesTheSame( baseA, baseB, *it ) == false )
            {
                return false;
            }
        }

        // Traverse into parent class (if there is one)
        ri = ri->GetSuperClass();
    }
    while( ri );

    return true;
}

// AreNodesTheSame
//------------------------------------------------------------------------------
/*static*/ bool NodeGraph::AreNodesTheSame( const void * baseA, const void * baseB, const ReflectedProperty & property )
{
    if ( property.HasMetaData< Meta_IgnoreForComparison >() )
    {
        return true;
    }

    switch ( property.GetType() )
    {
        case PropertyType::PT_ASTRING:
        {
            if ( property.IsArray() )
            {
                const Array< AString > * stringsA = property.GetPtrToArray<AString>( baseA );
                const Array< AString > * stringsB = property.GetPtrToArray<AString>( baseB );
                if ( stringsA->GetSize() != stringsB->GetSize() )
                {
                    return false;
                }
                const size_t numStrings = stringsA->GetSize();
                for ( size_t i=0; i<numStrings; ++i )
                {
                    if ( (*stringsA)[ i ] != (*stringsB)[ i ] )
                    {
                        return false;
                    }
                }
            }
            else
            {
                const AString * stringA = property.GetPtrToProperty<AString>( baseA );
                const AString * stringB = property.GetPtrToProperty<AString>( baseB );
                if ( *stringA != *stringB )
                {
                    return false;
                }
            }
            break;
        }
        case PT_UINT8:
        {
            ASSERT( property.IsArray() == false );

            const uint8_t * u8A = property.GetPtrToProperty<uint8_t>( baseA );
            const uint8_t * u8B = property.GetPtrToProperty<uint8_t>( baseB );
            if ( *u8A != *u8B )
            {
                return false;
            }
            break;
        }
        case PT_INT32:
        {
            ASSERT( property.IsArray() == false );
            const int32_t * i32A = property.GetPtrToProperty<int32_t>( baseA );
            const int32_t * i32B = property.GetPtrToProperty<int32_t>( baseB );
            if ( *i32A != *i32B )
            {
                return false;
            }
            break;
        }
        case PT_UINT32:
        {
            ASSERT( property.IsArray() == false );

            const uint32_t * u32A = property.GetPtrToProperty<uint32_t>( baseA );
            const uint32_t * u32B = property.GetPtrToProperty<uint32_t>( baseB );
            if ( *u32A != *u32B )
            {
                return false;
            }
            break;
        }
        case PT_UINT64:
        {
            ASSERT( property.IsArray() == false );

            const uint64_t * u64A = property.GetPtrToProperty<uint64_t>( baseA );
            const uint64_t * u64B = property.GetPtrToProperty<uint64_t>( baseB );
            if ( *u64A != *u64B )
            {
                return false;
            }
            break;
        }
        case PT_BOOL:
        {
            ASSERT( property.IsArray() == false );

            const bool * bA = property.GetPtrToProperty<bool>( baseA );
            const bool * bB = property.GetPtrToProperty<bool>( baseB );
            if ( *bA != *bB )
            {
                return false;
            }
            break;
        }
        case PT_STRUCT:
        {
            const ReflectedPropertyStruct & propertyStruct = static_cast<const ReflectedPropertyStruct &>( property );
            if ( property.IsArray() )
            {
                const uint32_t numElementsA = (uint32_t)propertyStruct.GetArraySize( baseA );
                const uint32_t numElementsB = (uint32_t)propertyStruct.GetArraySize( baseB );
                if ( numElementsA != numElementsB )
                {
                    return false;
                }
                for ( uint32_t i=0; i<numElementsA; ++i )
                {
                    if ( AreNodesTheSame( propertyStruct.GetStructInArray( baseA, i ), propertyStruct.GetStructInArray( baseB, i ), propertyStruct.GetStructReflectionInfo() ) == false )
                    {
                        return false;
                    }
                }
            }
            else
            {
                if ( AreNodesTheSame( propertyStruct.GetStructBase( baseA ), propertyStruct.GetStructBase( baseB ), propertyStruct.GetStructReflectionInfo() ) == false )
                {
                    return false;
                }
            }
            break;
        }
        default: ASSERT( false ); // Unhandled
    }

    return true;
}

// DoDependenciesMatch
//------------------------------------------------------------------------------
bool NodeGraph::DoDependenciesMatch( const Dependencies & depsA, const Dependencies & depsB )
{
    if ( depsA.GetSize() != depsB.GetSize() )
    {
        return false;
    }

    const size_t numDeps = depsA.GetSize();
    for ( size_t i = 0; i<numDeps; ++i )
    {
        const Node * nodeA = depsA[ i ].GetNode();
        const Node * nodeB = depsB[ i ].GetNode();
        if ( nodeA->GetType() != nodeB->GetType() )
        {
            return false;
        }
        if ( nodeA->GetStamp() != nodeB->GetStamp() )
        {
            return false;
        }
        if ( nodeA->GetName() != nodeB->GetName() )
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
