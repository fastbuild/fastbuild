// Node.cpp - base interface for dependency graph nodes
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Node.h"
#include "FileNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyDirNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CSNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ListDependenciesNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeProxy.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/RemoveDirNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"
#include "Tools/FBuild/FBuildCore/Graph/TextFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VSProjectBaseNode.h"
#include "Tools/FBuild/FBuildCore/Graph/XCodeProjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_AllowNonFile.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_EmbedMembers.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_IgnoreForComparison.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_InheritFromOwner.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_Name.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Mutex.h"
#include "Core/Profile/Profile.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdio.h>

// Static Data
//------------------------------------------------------------------------------
/*static*/ const char * const Node::s_NodeTypeNames[] =
{
    "Proxy",
    "CopyFile",
    "Directory",
    "Exec",
    "File",
    "Library",
    "Object",
    "Alias",
    "Exe",
    "Unity",
    "C#",
    "Test",
    "Compiler",
    "DLL",
    "VCXProj",
    "ObjectList",
    "CopyDir",
    "SLN",
    "RemoveDir",
    "XCodeProj",
    "Settings",
    "VSExtProj",
    "TextFile",
    "ListDependencies",
};
static Mutex g_NodeEnvStringMutex;

// Custom MetaData
//------------------------------------------------------------------------------
IMetaData & MetaName( const char * name )
{
    return *FNEW( Meta_Name( name ) );
}
IMetaData & MetaAllowNonFile()
{
    return *FNEW( Meta_AllowNonFile() );
}
IMetaData & MetaAllowNonFile( const Node::Type limitToType )
{
    return *FNEW( Meta_AllowNonFile( limitToType ) );
}
IMetaData & MetaEmbedMembers()
{
    return *FNEW( Meta_EmbedMembers() );
}
IMetaData & MetaInheritFromOwner()
{
    return *FNEW( Meta_InheritFromOwner() );
}
IMetaData & MetaIgnoreForComparison()
{
    return *FNEW( Meta_IgnoreForComparison() );
}

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_ABSTRACT( Node, Struct, MetaNone() )
REFLECT_END( Node )

// CONSTRUCTOR
//------------------------------------------------------------------------------
Node::Node( const AString & name, Type type, uint8_t controlFlags )
{
    m_Type = type;
    m_ControlFlags = controlFlags;

    SetName( name );

    // Compile time check to ensure name vector is in sync
    static_assert( sizeof( s_NodeTypeNames ) / sizeof(const char *) == NUM_NODE_TYPES, "s_NodeTypeNames item count doesn't match NUM_NODE_TYPES" );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Node::~Node() = default;

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool Node::DoDynamicDependencies( NodeGraph &, bool )
{
    return true;
}

// DetermineNeedToBuild
//------------------------------------------------------------------------------
bool Node::DetermineNeedToBuild( const Dependencies & deps ) const
{
    // Some nodes (like File and Directory) always build as they represent external state
    // that can be modified before the build is run
    if ( m_ControlFlags & FLAG_ALWAYS_BUILD )
    {
        // Don't output detailed FLOG_INFO for these nodes
        return true;
    }

    // if we don't have a stamp, we are building for the first time
    // can also occur if explicitly dirtied in a previous build
    if ( m_Stamp == 0 )
    {
        FLOG_BUILD_REASON( "Need to build '%s' (first time or dirtied)\n", GetName().Get() );
        return true;
    }

    if ( IsAFile() )
    {
        uint64_t lastWriteTime = FileIO::GetFileLastWriteTime( m_Name );

        if ( lastWriteTime == 0 )
        {
            // file is missing on disk
            FLOG_BUILD_REASON( "Need to build '%s' (missing)\n", GetName().Get() );
            return true;
        }

        if ( lastWriteTime != m_Stamp )
        {
            // on disk file doesn't match our file
            // (modified by some external process)
            FLOG_BUILD_REASON( "Need to build '%s' (externally modified - stamp = %" PRIu64 ", disk = %" PRIu64 ")\n", GetName().Get(), m_Stamp, lastWriteTime );
            return true;
        }
    }

    // static deps
    for ( const Dependency & dep : deps )
    {
        // Weak dependencies don't cause rebuilds
        if ( dep.IsWeak() )
        {
            continue;
        }

        const Node * n = dep.GetNode();

        const uint64_t stamp = n->GetStamp();
        if ( stamp == 0 )
        {
            // file missing - this may be ok, but node needs to build to find out
            FLOG_BUILD_REASON( "Need to build '%s' (dep missing: '%s')\n", GetName().Get(), n->GetName().Get() );
            return true;
        }

        // Compare the "stamp" for this dependency recorded last time we built. If it has changed
        // the dependency has changed and we must rebuild
        const uint64_t oldStamp = dep.GetNodeStamp();
        if ( stamp != oldStamp )
        {
            FLOG_BUILD_REASON( "Need to build '%s' (dep changed: '%s', %" PRIu64 " -> %" PRIu64 ")\n", GetName().Get(), n->GetName().Get(), oldStamp, stamp );
            return true;
        }
    }

    // nothing needs building
    return false;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult Node::DoBuild( Job * /*job*/ )
{
    ASSERT( false ); // Derived class is missing implementation
    return Node::NODE_RESULT_FAILED;
}

// DoBuild2
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult Node::DoBuild2( Job * /*job*/, bool /*racingRemoteJob*/ )
{
    ASSERT( false ); // Derived class is missing implementation
    return Node::NODE_RESULT_FAILED;
}

// Finalize
//------------------------------------------------------------------------------
/*virtual*/ bool Node::Finalize( NodeGraph & )
{
    // Stamp static and dynamic dependencies (prebuild deps don't need stamping
    // as they are never trigger builds)
    Dependencies * allDeps[2] = { &m_StaticDependencies, &m_DynamicDependencies };
    for ( Dependencies * deps : allDeps )
    {
        for ( Dependency & dep : *deps )
        {
            // If not built, each node should have a non-zero node stamp
            // If built, it's possible to have a zero stamp due to missing files
            ASSERT( dep.GetNode()->GetStatFlag( Node::STATS_BUILT ) ||
                    dep.GetNode()->GetStamp() );
            dep.Stamp( dep.GetNode()->GetStamp() );
        }
    }

    return true;
}

// EnsurePathExistsForFile
//------------------------------------------------------------------------------
/*static*/ bool Node::EnsurePathExistsForFile( const AString & name )
{
    const char * lastSlash = name.FindLast( NATIVE_SLASH );
    ASSERT( PathUtils::IsFullPath( name ) ); // should be guaranteed to be a full path
    AStackString<> pathOnly( name.Get(), lastSlash );
    if ( FileIO::EnsurePathExists( pathOnly ) == false )
    {
        FLOG_ERROR( "Failed to create path '%s'", pathOnly.Get() );
        return false;
    }
    return true;
}

// DoPreBuildFileDeletion
//------------------------------------------------------------------------------
/*static*/ bool Node::DoPreBuildFileDeletion( const AString & fileName )
{
    // Try to delete the file.
    if ( FileIO::FileDelete( fileName.Get() ) )
    {
        return true; // File deleted ok
    }

    // The common case is that the file exists (which is why we don't check to
    // see before deleting it above). If it failed to delete, we must now work
    // out if it's because it didn't exist or because of an actual problem.
    if ( FileIO::FileExists( fileName.Get() ) == false )
    {
        return true; // File didn't exist in the first place
    }

    // Couldn't delete the file
    FLOG_ERROR( "Failed to delete file before build '%s'", fileName.Get() );
    return false;
}

// GetLastBuildTime
//------------------------------------------------------------------------------
uint32_t Node::GetLastBuildTime() const
{
    return AtomicLoadRelaxed( &m_LastBuildTimeMs );
}

// SetLastBuildTime
//------------------------------------------------------------------------------
void Node::SetLastBuildTime( uint32_t ms )
{
    AtomicStoreRelaxed( &m_LastBuildTimeMs, ms );
}

// CreateNode
//------------------------------------------------------------------------------
/*static*/ Node * Node::CreateNode( NodeGraph & nodeGraph, Node::Type nodeType, const AString & name )
{
    switch ( nodeType )
    {
        case Node::PROXY_NODE:          ASSERT( false ); return nullptr;
        case Node::COPY_FILE_NODE:      return nodeGraph.CreateCopyFileNode( name );
        case Node::DIRECTORY_LIST_NODE: return nodeGraph.CreateDirectoryListNode( name );
        case Node::EXEC_NODE:           return nodeGraph.CreateExecNode( name );
        case Node::FILE_NODE:           return nodeGraph.CreateFileNode( name );
        case Node::LIBRARY_NODE:        return nodeGraph.CreateLibraryNode( name );
        case Node::OBJECT_NODE:         return nodeGraph.CreateObjectNode( name );
        case Node::ALIAS_NODE:          return nodeGraph.CreateAliasNode( name );
        case Node::EXE_NODE:            return nodeGraph.CreateExeNode( name );
        case Node::CS_NODE:             return nodeGraph.CreateCSNode( name );
        case Node::UNITY_NODE:          return nodeGraph.CreateUnityNode( name );
        case Node::TEST_NODE:           return nodeGraph.CreateTestNode( name );
        case Node::COMPILER_NODE:       return nodeGraph.CreateCompilerNode( name );
        case Node::DLL_NODE:            return nodeGraph.CreateDLLNode( name );
        case Node::VCXPROJECT_NODE:     return nodeGraph.CreateVCXProjectNode( name );
        case Node::VSPROJEXTERNAL_NODE: return nodeGraph.CreateVSProjectExternalNode( name );
        case Node::OBJECT_LIST_NODE:    return nodeGraph.CreateObjectListNode( name );
        case Node::COPY_DIR_NODE:       return nodeGraph.CreateCopyDirNode( name );
        case Node::SLN_NODE:            return nodeGraph.CreateSLNNode( name );
        case Node::REMOVE_DIR_NODE:     return nodeGraph.CreateRemoveDirNode( name );
        case Node::XCODEPROJECT_NODE:   return nodeGraph.CreateXCodeProjectNode( name );
        case Node::SETTINGS_NODE:       return nodeGraph.CreateSettingsNode( name );
        case Node::TEXT_FILE_NODE:      return nodeGraph.CreateTextFileNode( name );
        case Node::LIST_DEPENDENCIES_NODE: return nodeGraph.CreateListDependenciesNode( name );
        case Node::NUM_NODE_TYPES:      ASSERT( false ); return nullptr;
    }

    #if defined( __GNUC__ ) || defined( _MSC_VER )
        // GCC and incorrectly reports reaching end of non-void function (as of GCC 7.3.0)
        // MSVC incorrectly reports reaching end of non-void function (as of VS 2017)
        return nullptr;
    #endif
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * Node::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    // read type
    uint8_t nodeType;
    if ( stream.Read( nodeType ) == false )
    {
        return nullptr;
    }

    PROFILE_SECTION( Node::GetTypeName( (Type)nodeType ) );

    // Name of node
    AStackString<> name;
    if ( stream.Read( name ) == false )
    {
        return nullptr;
    }

    // Create node
    Node * n = CreateNode( nodeGraph, (Type)nodeType, name );
    ASSERT( n );

    // Early out for FileNode
    if ( nodeType == Node::FILE_NODE )
    {
        return n;
    }

    // Read stamp
    uint64_t stamp;
    if ( stream.Read( stamp ) == false )
    {
        return nullptr;
    }

    // Build time
    uint32_t lastTimeToBuild;
    if ( stream.Read( lastTimeToBuild ) == false )
    {
        return nullptr;
    }
    n->SetLastBuildTime( lastTimeToBuild );    

    // Dependencies
    if ( ( n->m_PreBuildDependencies.Load( nodeGraph, stream ) == false ) ||
         ( n->m_StaticDependencies.Load( nodeGraph, stream ) == false ) ||
         ( n->m_DynamicDependencies.Load( nodeGraph, stream ) == false ) )
    {
        return nullptr;
    }

    // Deserialize properties
    if ( Deserialize( stream, n, *n->GetReflectionInfoV() ) == false )
    {
        return nullptr;
    }

    n->PostLoad( nodeGraph ); // TODO:C Eliminate the need for this

    // set stamp
    n->m_Stamp = stamp;
    return n;
}

// PostLoad
//------------------------------------------------------------------------------
/*virtual*/ void Node::PostLoad( NodeGraph & /*nodeGraph*/ )
{
}

// Save
//------------------------------------------------------------------------------
/*static*/ void Node::Save( IOStream & stream, const Node * node )
{
    ASSERT( node );

    // save type
    uint8_t nodeType = node->GetType();
    stream.Write( nodeType );

    // Save Name
    stream.Write( node->m_Name );

    #if defined( DEBUG )
        node->MarkAsSaved();
    #endif

    // FileNodes don't need most things serialized:
    // - they have no dependencies (they are leaf nodes)
    // - they take sub 1ms to check, so don't need their build time saved
    // - their stamp is obtained every build, so doesn't need saving
    if ( nodeType == Node::FILE_NODE )
    {
        return;
    }

    // Stamp
    const uint64_t stamp = node->GetStamp();
    stream.Write( stamp );

    // Build time
    const uint32_t lastBuildTime = node->GetLastBuildTime();
    stream.Write( lastBuildTime );
    
    // Deps
    node->m_PreBuildDependencies.Save( stream );
    node->m_StaticDependencies.Save( stream );
    node->m_DynamicDependencies.Save( stream );

    // Properties
    const ReflectionInfo * const ri = node->GetReflectionInfoV();
    Serialize( stream, node, *ri );
}

// LoadRemote
//------------------------------------------------------------------------------
/*static*/ Node * Node::LoadRemote( IOStream & stream )
{
    // read type
    uint32_t nodeType;
    if ( stream.Read( nodeType ) == false )
    {
        return nullptr;
    }

    // read contents
    ASSERT( (Node::Type)nodeType == Node::OBJECT_NODE );
    return ObjectNode::LoadRemote( stream );
}

// SaveRemote
//------------------------------------------------------------------------------
/*static*/ void Node::SaveRemote( IOStream & stream, const Node * node )
{
    ASSERT( node );

    // only 1 type of node is ever serialized over the network
    ASSERT( node->GetType() == Node::OBJECT_NODE );

    // save type
    uint32_t nodeType = (uint32_t)node->GetType();
    stream.Write( nodeType );

    // save contents
    node->SaveRemote( stream );
}

// SaveRemote
//------------------------------------------------------------------------------
/*virtual*/ void Node::SaveRemote( IOStream & /*stream*/ ) const
{
    // Should never get here.  Either:
    // a) Derived Node is missing SaveRemote implementation
    // b) Serializing an unexpected type
    ASSERT( false );
}

// Serialize
//------------------------------------------------------------------------------
/*static*/ void Node::Serialize( IOStream & stream, const void * base, const ReflectionInfo & ri )
{
    const ReflectionInfo * currentRI = &ri;
    do
    {
        const ReflectionIter end = currentRI->End();
        for ( ReflectionIter it = currentRI->Begin(); it != end; ++it )
        {
            const ReflectedProperty & property = *it;
            Serialize( stream, base, property );
        }

        currentRI = currentRI->GetSuperClass();
    }
    while ( currentRI );
}

// Serialize
//------------------------------------------------------------------------------
/*static*/ void Node::Serialize( IOStream & stream, const void * base, const ReflectedProperty & property )
{
    const PropertyType pt = property.GetType();
    switch ( pt )
    {
        case PT_ASTRING:
        {
            if ( property.IsArray() )
            {
                const Array< AString > * arrayOfStrings = property.GetPtrToArray<AString>( base );
                VERIFY( stream.Write( *arrayOfStrings ) );
            }
            else
            {
                const AString * string = property.GetPtrToProperty<AString>( base );
                VERIFY( stream.Write( *string ) );
            }
            return;
        }
        case PT_BOOL:
        {
            bool b( false );
            property.GetProperty( base, &b );
            VERIFY( stream.Write( b ) );
            return;
        }
        case PT_UINT8:
        {
            uint8_t u8( 0 );
            property.GetProperty( base, &u8 );
            VERIFY( stream.Write( u8 ) );
            return;
        }
        case PT_INT32:
        {
            int32_t i32( 0 );
            property.GetProperty( base, &i32 );
            VERIFY( stream.Write( i32 ) );
            return;
        }
        case PT_UINT32:
        {
            uint32_t u32( 0 );
            property.GetProperty( base, &u32 );
            VERIFY( stream.Write( u32 ) );
            return;
        }
        case PT_UINT64:
        {
            uint64_t u64( 0 );
            property.GetProperty( base, &u64 );
            VERIFY( stream.Write( u64 ) );
            return;
        }
        case PT_STRUCT:
        {
            const ReflectedPropertyStruct & propertyS = static_cast< const ReflectedPropertyStruct & >( property );

            if ( property.IsArray() )
            {
                // Write number of elements
                const uint32_t numElements = (uint32_t)propertyS.GetArraySize( base );
                VERIFY( stream.Write( numElements ) );

                // Write each element
                for ( uint32_t i=0; i<numElements; ++i )
                {
                    const void * structBase = propertyS.GetStructInArray( base, (size_t)i );
                    Serialize( stream, structBase, *propertyS.GetStructReflectionInfo() );
                }
            }
            else
            {
                const ReflectionInfo * structRI = propertyS.GetStructReflectionInfo();
                const void * structBase = propertyS.GetStructBase( base );
                Serialize( stream, structBase, *structRI );
            }
            return;
        }
        default:
        {
            break; // Fall through to error
        }
    }
    ASSERT( false ); // Unsupported type
}

// Deserialize
//------------------------------------------------------------------------------
/*static*/ bool Node::Deserialize( IOStream & stream, void * base, const ReflectionInfo & ri )
{
    const ReflectionInfo * currentRI = &ri;
    do
    {
        const ReflectionIter end = currentRI->End();
        for ( ReflectionIter it = currentRI->Begin(); it != end; ++it )
        {
            const ReflectedProperty & property = *it;
            if ( !Deserialize( stream, base, property ) )
            {
                return false;
            }
        }

        currentRI = currentRI->GetSuperClass();
    }
    while ( currentRI );

    return true;
}

// Migrate
//------------------------------------------------------------------------------
/*virtual*/ void Node::Migrate( const Node & oldNode )
{
    // Transfer the stamp used to detemine if the node has changed
    m_Stamp = oldNode.m_Stamp;

    // Transfer previous build costs used for progress estimates
    m_LastBuildTimeMs = oldNode.m_LastBuildTimeMs;
}

// Deserialize
//------------------------------------------------------------------------------
/*static*/ bool Node::Deserialize( IOStream & stream, void * base, const ReflectedProperty & property )
{
    const PropertyType pt = property.GetType();
    switch ( pt )
    {
        case PT_ASTRING:
        {
            if ( property.IsArray() )
            {
                Array< AString > * arrayOfStrings = property.GetPtrToArray<AString>( base );
                if ( stream.Read( *arrayOfStrings ) == false )
                {
                    return false;
                }
            }
            else
            {
                AString * string = property.GetPtrToProperty<AString>( base );
                if ( stream.Read( *string ) == false )
                {
                    return false;
                }
            }
            return true;
        }
        case PT_BOOL:
        {
            bool b( false );
            if ( stream.Read( b ) == false )
            {
                return false;
            }
            property.SetProperty( base, b );
            return true;
        }
        case PT_UINT8:
        {
            uint8_t u8( 0 );
            if ( stream.Read( u8 ) == false )
            {
                return false;
            }
            property.SetProperty( base, u8 );
            return true;
        }
        case PT_INT32:
        {
            int32_t i32( 0 );
            if ( stream.Read( i32 ) == false )
            {
                return false;
            }
            property.SetProperty( base, i32 );
            return true;
        }
        case PT_UINT32:
        {
            uint32_t u32( 0 );
            if ( stream.Read( u32 ) == false )
            {
                return false;
            }
            property.SetProperty( base, u32 );
            return true;
        }
        case PT_UINT64:
        {
            uint64_t u64( 0 );
            if ( stream.Read( u64 ) == false )
            {
                return false;
            }
            property.SetProperty( base, u64 );
            return true;
        }
        case PT_STRUCT:
        {
            const ReflectedPropertyStruct & propertyS = static_cast< const ReflectedPropertyStruct & >( property );

            if ( property.IsArray() )
            {
                // Read number of elements
                uint32_t numElements( 0 );
                if ( stream.Read( numElements ) == false )
                {
                    return false;
                }
                propertyS.ResizeArrayOfStruct( base, numElements );

                // Read each element
                for ( uint32_t i=0; i<numElements; ++i )
                {
                    void * structBase = propertyS.GetStructInArray( base, (size_t)i );
                    if ( Deserialize( stream, structBase, *propertyS.GetStructReflectionInfo() ) == false )
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                const ReflectionInfo * structRI = propertyS.GetStructReflectionInfo();
                void * structBase = propertyS.GetStructBase( base );
                return Deserialize( stream, structBase, *structRI );
            }
        }
        default:
        {
            break; // Fall through to error
        }
    }

    ASSERT( false ); // Unsupported type
    return false;
}

// SetName
//------------------------------------------------------------------------------
void Node::SetName( const AString & name )
{
    m_Name = name;
    m_NameCRC = CRC32::CalcLower( name );
}

// ReplaceDummyName
//------------------------------------------------------------------------------
void Node::ReplaceDummyName( const AString & newName )
{
    m_Name = newName;
}

// DumpOutput
//------------------------------------------------------------------------------
/*static*/ void Node::DumpOutput( Job * job,
                                  const AString & output,
                                  const Array< AString > * exclusions )
{
    if ( output.IsEmpty() )
    {
        return;
    }

    // preallocate a large buffer
    AString buffer( MEGABYTE );

    const char * data = output.Get();
    const char * end = output.GetEnd();
    while( data < end )
    {
        // find the limits of the current line
        const char * lineStart = data;
        const char * lineEnd = lineStart;
        while ( lineEnd < end )
        {
            if ( ( *lineEnd == '\r' ) || ( *lineEnd == '\n' ) )
            {
                break;
            }
            lineEnd++;
        }
        if ( lineStart != lineEnd ) // ignore empty
        {
            // make a copy of the line to output
            AStackString< 1024 > copy( lineStart, lineEnd );

            // skip this line?
            bool skip = false;
            if ( exclusions )
            {
                const AString * iter = exclusions->Begin();
                const AString * const endIter = exclusions->End();
                while ( iter != endIter )
                {
                    if ( copy.BeginsWith( *iter ) )
                    {
                        skip = true;
                        break;
                    }
                    iter++;
                }
            }
            if ( !skip )
            {
                copy += '\n';

                // Clang format fixup for Visual Studio
                // (FBuild is null in remote context - fixup occurs on fbuild client machine)
                if ( FBuild::IsValid() && FBuild::Get().GetOptions().m_FixupErrorPaths )
                {
                    FixupPathForVSIntegration( copy );
                }

                // if the buffer needs to grow, grow in 1MiB chunks
                if ( buffer.GetLength() + copy.GetLength() > buffer.GetReserved() )
                {
                    buffer.SetReserved( buffer.GetReserved() + MEGABYTE );
                }
                buffer += copy;
            }
        }
        data = ( lineEnd + 1 );
    }

    if ( job == nullptr )
    {
        // Log directly to stdout since we have no job
        FLOG_ERROR_DIRECT( buffer.Get() );
    }
    else
    {
        // Log via job
        job->ErrorPreformatted( buffer.Get() );
    }
}

// FixupPathForVSIntegration
//------------------------------------------------------------------------------
/*static*/ void Node::FixupPathForVSIntegration( AString & line )
{
    // Clang/GCC Style
    // Convert:
    //     Core/Mem/Mem.h:23:1: warning: some warning text
    //     .\Tools/FBuild/FBuildCore/Graph/Node.h(294,24): warning: some warning text
    // To:
    //     <path>\Core\Mem\Mem.h(23,1): warning: some warning text
    //
    {
        const char * tag = line.Find( ": warning:" );
        tag = tag ? tag : line.Find( ": note:" );
        tag = tag ? tag : line.Find( ": error:" );
        tag = tag ? tag : line.Find( ": fatal error:" );
        tag = tag ? tag : line.Find( ": remark:" );
        if ( tag )
        {
            FixupPathForVSIntegration_GCC( line, tag );
            return;
        }
    }

    // SNC Style
    // Convert:
    //     Core/Mem/Mem.h(23,1): warning 55: some warning text
    // To:
    //     <path>\Core\Mem\Mem.h(23,1): warning 55: some warning text
    //
    {
        const char * tag = line.Find( ": error " );
        tag = tag ? tag : line.Find( ": warning " );
        tag = tag ? tag : line.Find( ": note " );
        tag = tag ? tag : line.Find( ": remark " );
        if ( tag )
        {
            FixupPathForVSIntegration_SNC( line, tag );
            return;
        }
    }

    // VBCC Style
    // Convert:
    //     warning 55 in line 23 of "Core/Mem/Mem.h": some warning text
    // To:
    //     <path>\Core\Mem\Mem.h(23,1): warning 55: some warning text
    //
    {
        const char * tag = ( line.BeginsWith( "warning " ) ? line.Get() : nullptr );
        tag = tag ? tag : ( line.BeginsWith( "error " ) ? line.Get() : nullptr );
        if ( tag )
        {
            FixupPathForVSIntegration_VBCC( line, tag );
            return;
        }
    }

    // leave line untouched
}

// FixupPathForVSIntegration_GCC
//------------------------------------------------------------------------------
/*static*/ void Node::FixupPathForVSIntegration_GCC( AString & line, const char * tag )
{
    AStackString<> beforeTag( line.Get(), tag );

    // is the error position in (x,y) style? (As opposed to :x:y: style)
    const bool commaStyle = ( ( beforeTag.Find( ':' ) == nullptr ) && beforeTag.Find( ',' ) );
    if ( commaStyle )
    {
        // Convert brace style to : style
        const uint32_t count = beforeTag.Replace( '(', ':' ) +
                               beforeTag.Replace( ',', ':' ) +
                               beforeTag.Replace( ')', ':' );
        if ( count != 3 )
        {
            return; // Unexpected format
        }
    }

    Array< AString > tokens;
    beforeTag.Tokenize( tokens, ':' );
    const size_t numTokens = tokens.GetSize();
    if ( numTokens < 3 )
    {
        return; // not enough : to be correctly formatted
    }

    // are last two tokens numbers?
    int row, column;
    PRAGMA_DISABLE_PUSH_MSVC( 4996 ) // This function or variable may be unsafe...
    PRAGMA_DISABLE_PUSH_CLANG_WINDOWS( "-Wdeprecated-declarations" ) // 'sscanf' is deprecated: This function or variable may be unsafe...
    if ( ( sscanf( tokens[ numTokens - 1 ].Get(), "%i", &column ) != 1 ) || // TODO:C Consider using sscanf_s
         ( sscanf( tokens[ numTokens - 2 ].Get(), "%i", &row ) != 1 ) ) // TODO:C Consider using sscanf_s
    PRAGMA_DISABLE_POP_CLANG_WINDOWS // -Wdeprecated-declarations
    PRAGMA_DISABLE_POP_MSVC // 4996
    {
        return; // failed to extract numbers where we expected them
    }

    // rebuild fixed string
    AStackString<> fixed;

    // convert path if needed
    if ( tokens[ 0 ].GetLength() == 1 )
    {
        ASSERT( numTokens >= 4 ); // should have an extra token due to ':'
        fixed = tokens[ 0 ]; // already a full path
    }
    else
    {
        NodeGraph::CleanPath( tokens[ 0 ], fixed );
    }

    // insert additional tokens
    for ( size_t i=1; i<( numTokens-2 ); ++i )
    {
        fixed += ':';
        fixed += tokens[ i ];
    }

    // format row, column
    fixed += '(';
    fixed += tokens[ numTokens - 2 ];
    fixed += ',';
    fixed += tokens[ numTokens - 1 ];
    fixed += ')';

    // remained of msg is untouched
    fixed += tag;

    line = fixed;
}

// FixupPathForVSIntegration_SNC
//------------------------------------------------------------------------------
/*static*/ void Node::FixupPathForVSIntegration_SNC( AString & line, const char * tag )
{
    AStackString<> beforeTag( line.Get(), tag );

    const char * openBracket = beforeTag.Find( '(' );
    if ( openBracket == nullptr )
    {
        return; // failed to find bracket where expected
    }

    AStackString<> path( beforeTag.Get(), openBracket );
    AStackString<> fixed;
    NodeGraph::CleanPath( path, fixed );

    // add back row, column
    fixed += openBracket;

    // remainder of msg is untouched
    fixed += tag;

    line = fixed;
}

// FixupPathForVSIntegration_VBCC
//------------------------------------------------------------------------------
/*static*/ void Node::FixupPathForVSIntegration_VBCC( AString & line, const char * /*tag*/ )
{
    Array< AString > tokens;
    line.Tokenize( tokens, ' ' );

    //     warning 55 in line 8 of "Core/Mem/Mem.h": some warning text
    if ( tokens.GetSize() < 9 )
    {
        return; // A line we don't expect, ignore it
    }
    ASSERT( ( tokens[ 0 ] == "warning" ) || ( tokens[ 0 ] == "error" ) );

    // Only try to fixup "line" errors and not other errors like:
    // - warning 65 in function "Blah": var <x> was never used
    if ( tokens[ 3 ] != "line" )
    {
        return;
    }
    // Ignore warnings from the underlying assembler such as:
    // - warning 2006 in line 307: bad extension - using default
    if ( tokens[5] != "of" )
    {
        return;
    }

    const char * problemType = tokens[ 0 ].Get(); // Warning or error
    const char * warningNum = tokens[ 1 ].Get();
    const char * warningLine = tokens[ 4 ].Get();
    AStackString<> fileName( tokens[ 6 ] );
    if ( fileName.BeginsWith( '"' ) && fileName.EndsWith( "\":" ) )
    {
        fileName.Trim( 1, 2 );
    }
    NodeGraph::CleanPath( fileName );

    //     <path>\Core\Mem\Mem.h(23,1): warning 55: some warning text
    AStackString<> buffer;
    buffer.Format( "%s(%s,1): %s %s: ", fileName.Get(), warningLine, problemType, warningNum );

    // add rest of warning
    for ( size_t i=7; i < tokens.GetSize(); ++i )
    {
        buffer += tokens[ i ];
        buffer += ' ';
    }
    line = buffer;
}

// InitializePreBuildDependencies
//------------------------------------------------------------------------------
bool Node::InitializePreBuildDependencies( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function, const Array< AString > & preBuildDependencyNames )
{
    if ( preBuildDependencyNames.IsEmpty() )
    {
        return true;
    }

    // Pre-size hint
    m_PreBuildDependencies.SetCapacity( preBuildDependencyNames.GetSize() );

    // Expand
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".PreBuildDependencies", preBuildDependencyNames, m_PreBuildDependencies, true, true, true, true ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    return true;
}

// GetEnvironmentString
//------------------------------------------------------------------------------
/*static*/ const char * Node::GetEnvironmentString( const Array< AString > & envVars,
                                                    const char * & inoutCachedEnvString )
{
    // If we've previously built a custom env string, use it
    if ( inoutCachedEnvString )
    {
        return inoutCachedEnvString;
    }

    // Do we need a custom env string?
    if ( envVars.IsEmpty() )
    {
        // No - return build-wide environment
        return FBuild::IsValid() ? FBuild::Get().GetEnvironmentString() : nullptr;
    }

    // More than one caller could be retrieving the same env string
    // in some cases. For simplicity, we protect in all cases even
    // if we could avoid it as the mutex will not be heavily constested.
    MutexHolder mh( g_NodeEnvStringMutex );

    // Caller owns thr memory
    inoutCachedEnvString = Env::AllocEnvironmentString( envVars );
    return inoutCachedEnvString;
}

// RecordStampFromBuiltFile
//------------------------------------------------------------------------------
void Node::RecordStampFromBuiltFile()
{
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    
    // An external tool might fail to write a file. Higher level code checks for
    // that (see "missing despite success"), so we don't need to do anything here.
    if ( m_Stamp == 0 )
    {
        return;
    }
    
    // On OS X, the 'ar' tool (for making libraries) appears to clamp the
    // modification time of libraries to whole seconds. On HFS/HFS+ file systems,
    // this doesn't matter because the resolution of the file system is 1 second.
    //
    // As of OS X 10.13 (High Sierra) Apple added a new filesystem (APFS) and
    // this is the default for all drives on 10.14 (Mojave). This filesystem
    // supports nanosecond filetime granularity.
    //
    // The combination of these things means that on an APFS file system a library
    // built after an object file can have a time that is older. Because
    // FASTBuild expects chronologically sensible filetimes, this backwards
    // time relationship triggers unnecessary builds.
    //
    // As a work-around, if we detect that a file has a modification which is
    // precisely a multiple of 1 second, we manually update the timestamp to
    // the current time.
    //
    // TODO:B Remove this work-around. A planned change to the dependency db
    // to record times per dependency and see when the differ instead of when
    // they are more recent will fix this.
    #if defined( __OSX__ )
        // For now, only apply the work-around to library nodes
        if ( GetType() == Node::LIBRARY_NODE )
        {
            if ( ( m_Stamp % 1000000000 ) == 0 )
            {
                // Set to current time
                FileIO::SetFileLastWriteTimeToNow( m_Name );
                
                // Re-query the time from the file
                m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
                ASSERT( m_Stamp != 0 );
            }
        }
    #endif
}

//------------------------------------------------------------------------------
