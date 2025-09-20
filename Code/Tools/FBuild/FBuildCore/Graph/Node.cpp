// Node.cpp - base interface for dependency graph nodes
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Node.h"
#include "FileNode.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CSNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyDirNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExeNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ExecNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ListDependenciesNode.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_AllowNonFile.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_EmbedMembers.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_IgnoreForComparison.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_InheritFromOwner.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_Name.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeProxy.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/RemoveDirNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SettingsNode.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"
#include "Tools/FBuild/FBuildCore/Graph/TextFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VSProjectBaseNode.h"
#include "Tools/FBuild/FBuildCore/Graph/XCodeProjectNode.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/xxHash.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Mutex.h"
#include "Core/Profile/Profile.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdio.h>

// Static Data
//------------------------------------------------------------------------------
/*static*/ uint32_t Node::s_SecondaryTag = 0;
// clang-format off
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
// clang-format on
static Mutex g_NodeEnvStringMutex;

//------------------------------------------------------------------------------
namespace
{
#pragma pack( push, 1 )
    class SerializedNodeBasic
    {
    public:
        uint8_t m_Type;
        uint32_t m_NameHash;
    };

    class SerializedNodeExtended
    {
    public:
        uint64_t m_Stamp;
        uint32_t m_LastBuildTime;
        uint32_t m_NumPreBuildDeps;
        uint32_t m_NumStaticDeps;
        uint32_t m_NumDynamicDeps;
    };
#pragma pack( pop )
}

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
Node::Node( Type type )
{
    m_Type = type;

    // Compile time check to ensure name vector is in sync
    static_assert( sizeof( s_NodeTypeNames ) / sizeof( const char * ) == NUM_NODE_TYPES, "s_NodeTypeNames item count doesn't match NUM_NODE_TYPES" );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Node::~Node() = default;

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool Node::DoDynamicDependencies( NodeGraph & /*nodeGraph*/ )
{
    return true;
}

// DetermineNeedToBuildStatic
//------------------------------------------------------------------------------
/*virtual*/ bool Node::DetermineNeedToBuildStatic() const
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

    // Handle missing or modified files
    if ( IsAFile() )
    {
        const uint64_t lastWriteTime = FileIO::GetFileLastWriteTime( m_Name );

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

    // Check if anything we statically depend on has changed
    return DetermineNeedToBuild( m_StaticDependencies );
}

// DetermineNeedToBuildDynamic
//------------------------------------------------------------------------------
/*virtual*/ bool Node::DetermineNeedToBuildDynamic() const
{
    // Should never be called if DetermineNeedToBuildStatic would trigger a build
    ASSERT( ( m_ControlFlags & FLAG_ALWAYS_BUILD ) == 0 );
    ASSERT( m_Stamp != 0 );

    // Check if anything we dynamically depend on has changed
    return DetermineNeedToBuild( m_DynamicDependencies );
}

// DetermineNeedToBuild
//------------------------------------------------------------------------------
bool Node::DetermineNeedToBuild( const Dependencies & deps ) const
{
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
    return BuildResult::eFailed;
}

// DoBuild2
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult Node::DoBuild2( Job * /*job*/, bool /*racingRemoteJob*/ )
{
    ASSERT( false ); // Derived class is missing implementation
    return BuildResult::eFailed;
}

// Finalize
//------------------------------------------------------------------------------
/*virtual*/ bool Node::Finalize( NodeGraph & )
{
    // Stamp static and dynamic dependencies (prebuild deps don't need stamping
    // as they are never trigger builds)
    Dependencies * allDeps[ 2 ] = { &m_StaticDependencies, &m_DynamicDependencies };
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
    AStackString pathOnly( name.Get(), lastSlash );
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

//------------------------------------------------------------------------------
/*static*/ void Node::Load( NodeGraph & nodeGraph, ConstMemoryStream & stream )
{
    // Name of node
    AString name; // Will be moved
    VERIFY( stream.Read( name ) );

    // Use data directly from memory buffer
    const uint64_t pos = stream.Tell();
    const char * data = static_cast<const char *>( stream.GetData() );
    const SerializedNodeBasic & info = *reinterpret_cast<const SerializedNodeBasic *>( data + pos );

    // Consume common attributes
    VERIFY( stream.Seek( pos + sizeof( SerializedNodeBasic ) ) );

    // Create node
    VERIFY( nodeGraph.CreateNode( static_cast<Type>( info.m_Type ),
                                  Move( name ),
                                  info.m_NameHash ) );
}

//------------------------------------------------------------------------------
/*static*/ void Node::LoadExtended( NodeGraph & nodeGraph,
                                    Node * node,
                                    ConstMemoryStream & stream )
{
    // Should not be called on FileNode
    ASSERT( node->GetType() != Node::FILE_NODE );

    // Use data directly from memory buffer
    const uint64_t pos = stream.Tell();
    const char * data = static_cast<const char *>( stream.GetData() );
    const SerializedNodeExtended & info = *reinterpret_cast<const SerializedNodeExtended *>( data + pos );

    // Consume extended data
    VERIFY( stream.Seek( pos + sizeof( SerializedNodeExtended ) ) );

    // Build time
    node->SetLastBuildTime( info.m_LastBuildTime );

    // Deserialize properties
    Deserialize( nodeGraph, stream, node, *node->GetReflectionInfoV() );

    // set stamp
    node->m_Stamp = info.m_Stamp;

    // Dependencies
    node->m_PreBuildDependencies.Load( nodeGraph, info.m_NumPreBuildDeps, stream );
    node->m_StaticDependencies.Load( nodeGraph, info.m_NumStaticDeps, stream );
    node->m_DynamicDependencies.Load( nodeGraph, info.m_NumDynamicDeps, stream );
}

// PostLoad
//------------------------------------------------------------------------------
/*virtual*/ void Node::PostLoad( NodeGraph & /*nodeGraph*/ )
{
}

//------------------------------------------------------------------------------
/*static*/ void Node::Save( IOStream & stream, const Node * node )
{
    ASSERT( node );

    // Save Name
    stream.Write( node->m_Name );

    // Save common attributes
    SerializedNodeBasic info;
    info.m_Type = node->GetType();
    info.m_NameHash = node->m_NameHash;
    VERIFY( stream.WriteBuffer( &info, sizeof( SerializedNodeBasic ) ) == sizeof( SerializedNodeBasic ) );
}

//------------------------------------------------------------------------------
/*static*/ void Node::SaveExtended( IOStream & stream, const Node * node )
{
    // Should not be called on FileNode
    ASSERT( node->GetType() != Node::FILE_NODE );

    // Prep extended data
    SerializedNodeExtended info;
    info.m_Stamp = node->GetStamp();
    info.m_LastBuildTime = node->GetLastBuildTime();
    info.m_NumPreBuildDeps = static_cast<uint32_t>( node->m_PreBuildDependencies.GetSize() );
    info.m_NumStaticDeps = static_cast<uint32_t>( node->m_StaticDependencies.GetSize() );
    info.m_NumDynamicDeps = static_cast<uint32_t>( node->m_DynamicDependencies.GetSize() );

    // Save everything
    VERIFY( stream.WriteBuffer( &info, sizeof( SerializedNodeExtended ) ) == sizeof( SerializedNodeExtended ) );

    // Properties
    const ReflectionInfo * const ri = node->GetReflectionInfoV();
    Serialize( stream, node, *ri );

    // Deps
    node->m_PreBuildDependencies.Save( stream );
    node->m_StaticDependencies.Save( stream );
    node->m_DynamicDependencies.Save( stream );
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
    const uint32_t nodeType = (uint32_t)node->GetType();
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
    } while ( currentRI );
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
                const Array<AString> * arrayOfStrings = property.GetPtrToArray<AString>( base );
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
            const ReflectedPropertyStruct & propertyS = static_cast<const ReflectedPropertyStruct &>( property );

            if ( property.IsArray() )
            {
                // Write number of elements
                const uint32_t numElements = (uint32_t)propertyS.GetArraySize( base );
                VERIFY( stream.Write( numElements ) );

                // Write each element
                for ( uint32_t i = 0; i < numElements; ++i )
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
        case PT_CUSTOM_1:
        {
            const Node * node = *property.GetPtrToPropertyCustom<Node *>( base );
            const uint32_t nodeIndex = node->GetBuildPassTag();
            stream.Write( nodeIndex );
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
/*static*/ void Node::Deserialize( NodeGraph & nodeGraph,
                                   ConstMemoryStream & stream,
                                   void * base,
                                   const ReflectionInfo & ri )
{
    const ReflectionInfo * currentRI = &ri;
    do
    {
        const ReflectionIter end = currentRI->End();
        for ( ReflectionIter it = currentRI->Begin(); it != end; ++it )
        {
            const ReflectedProperty & property = *it;
            Deserialize( nodeGraph, stream, base, property );
        }

        currentRI = currentRI->GetSuperClass();
    } while ( currentRI );
}

// Migrate
//------------------------------------------------------------------------------
/*virtual*/ void Node::Migrate( const Node & oldNode )
{
    // Transfer the stamp used to determine if the node has changed
    m_Stamp = oldNode.m_Stamp;

    // Transfer previous build costs used for progress estimates
    m_LastBuildTimeMs = oldNode.m_LastBuildTimeMs;
}

// Deserialize
//------------------------------------------------------------------------------
/*static*/ void Node::Deserialize( NodeGraph & nodeGraph,
                                   ConstMemoryStream & stream,
                                   void * base,
                                   const ReflectedProperty & property )
{
    const PropertyType pt = property.GetType();
    switch ( pt )
    {
        case PT_ASTRING:
        {
            if ( property.IsArray() )
            {
                Array<AString> * arrayOfStrings = property.GetPtrToArray<AString>( base );
                VERIFY( stream.Read( *arrayOfStrings ) );
            }
            else
            {
                AString * string = property.GetPtrToProperty<AString>( base );
                VERIFY( stream.Read( *string ) );
            }
            return;
        }
        case PT_BOOL:
        {
            bool b( false );
            VERIFY( stream.Read( b ) );
            property.SetProperty( base, b );
            return;
        }
        case PT_UINT8:
        {
            uint8_t u8( 0 );
            VERIFY( stream.Read( u8 ) );
            property.SetProperty( base, u8 );
            return;
        }
        case PT_INT32:
        {
            int32_t i32( 0 );
            VERIFY( stream.Read( i32 ) );
            property.SetProperty( base, i32 );
            return;
        }
        case PT_UINT32:
        {
            uint32_t u32( 0 );
            VERIFY( stream.Read( u32 ) );
            property.SetProperty( base, u32 );
            return;
        }
        case PT_UINT64:
        {
            uint64_t u64( 0 );
            VERIFY( stream.Read( u64 ) );
            property.SetProperty( base, u64 );
            return;
        }
        case PT_STRUCT:
        {
            const ReflectedPropertyStruct & propertyS = static_cast<const ReflectedPropertyStruct &>( property );

            if ( property.IsArray() )
            {
                // Read number of elements
                uint32_t numElements( 0 );
                VERIFY( stream.Read( numElements ) );
                propertyS.ResizeArrayOfStruct( base, numElements );

                // Read each element
                for ( uint32_t i = 0; i < numElements; ++i )
                {
                    void * structBase = propertyS.GetStructInArray( base, (size_t)i );
                    Deserialize( nodeGraph, stream, structBase, *propertyS.GetStructReflectionInfo() );
                }
                return;
            }
            else
            {
                const ReflectionInfo * structRI = propertyS.GetStructReflectionInfo();
                void * structBase = propertyS.GetStructBase( base );
                Deserialize( nodeGraph, stream, structBase, *structRI );
                return;
            }
        }
        case PT_CUSTOM_1:
        {
            uint32_t index;
            VERIFY( stream.Read( index ) );
            Node * node = nodeGraph.GetNodeByIndex( index );
            ASSERT( node );
            Node ** propertyPtr = property.GetPtrToPropertyCustom<Node *>( base );
            *propertyPtr = node;
            return;
        }
        default:
        {
            break; // Fall through to error
        }
    }

    ASSERT( false ); // Unsupported type
}

// SetName
//------------------------------------------------------------------------------
void Node::SetName( AString && name, uint32_t nameHashHint )
{
    ASSERT( ( nameHashHint == 0 ) || ( nameHashHint == CalcNameHash( name ) ) );
    m_NameHash = nameHashHint ? nameHashHint : CalcNameHash( name );
    m_Name = Move( name );
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
                                  const Array<AString> * exclusions )
{
    if ( output.IsEmpty() )
    {
        return;
    }

    // preallocate a large buffer
    AString buffer( MEGABYTE );

    const char * data = output.Get();
    const char * end = output.GetEnd();
    while ( data < end )
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
            AStackString<1024> copy( lineStart, lineEnd );

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
    AStackString beforeTag( line.Get(), tag );

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

    StackArray<AString> tokens;
    beforeTag.Tokenize( tokens, ':' );
    const size_t numTokens = tokens.GetSize();
    if ( numTokens < 3 )
    {
        return; // not enough : to be correctly formatted
    }

    // are last two tokens numbers?
    int row, column;
    if ( ( tokens[ numTokens - 1 ].Scan( "%i", &column ) != 1 ) ||
         ( tokens[ numTokens - 2 ].Scan( "%i", &row ) != 1 ) )
    {
        return; // failed to extract numbers where we expected them
    }

    // rebuild fixed string
    AStackString fixed;

    // Normalize path
    CleanPathForVSIntegration( tokens[ 0 ], fixed );

    // insert additional tokens
    for ( size_t i = 1; i < ( numTokens - 2 ); ++i )
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
    AStackString beforeTag( line.Get(), tag );

    const char * openBracket = beforeTag.Find( '(' );
    if ( openBracket == nullptr )
    {
        return; // failed to find bracket where expected
    }

    // rebuild fixed string
    AStackString fixed;

    // Normalize path
    const AStackString path( beforeTag.Get(), openBracket );
    CleanPathForVSIntegration( path, fixed );

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
    StackArray<AString> tokens;
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
    if ( tokens[ 5 ] != "of" )
    {
        return;
    }

    const char * problemType = tokens[ 0 ].Get(); // Warning or error
    const char * warningNum = tokens[ 1 ].Get();
    const char * warningLine = tokens[ 4 ].Get();
    AStackString fileName( tokens[ 6 ] );
    if ( fileName.BeginsWith( '"' ) && fileName.EndsWith( "\":" ) )
    {
        fileName.Trim( 1, 2 );
    }

    // Rebuild fixed string
    AStackString fixed;

    // Normalize path
    CleanPathForVSIntegration( fileName, fixed );

    //     <path>\Core\Mem\Mem.h(23,1): warning 55: some warning text
    fixed.AppendFormat( "(%s,1): %s %s: ", warningLine, problemType, warningNum );

    // add rest of warning
    for ( size_t i = 7; i < tokens.GetSize(); ++i )
    {
        fixed += tokens[ i ];
        fixed += ' ';
    }
    line = fixed;
}

//------------------------------------------------------------------------------
/*static*/ void Node::CleanPathForVSIntegration( const AString & path, AString & outFixedPath )
{
    // If the path passed in is a single token, then it is considered to
    // be a drive letter (split by the : used as an error separator)
    if ( path.GetLength() == 1 )
    {
        outFixedPath = path;
        return;
    }

    // Is this a Linux path (under WSL for example)
    if ( ( path.GetLength() >= 7 ) &&
         path.BeginsWith( "/mnt/" ) &&
         ( path[ 6 ] == '/' ) )
    {
        const char driveLetter = path[ 5 ];

        // convert /mnt/X/... -> X:/...
        outFixedPath.AppendFormat( "%c:%s", driveLetter, ( path.Get() + 6 ) );

        // convert to Windows-style slashes
        outFixedPath.Replace( '/', '\\' );
        return;
    }

    // Normalize path
    NodeGraph::CleanPath( path, outFixedPath );
}

//------------------------------------------------------------------------------
/*virtual*/ uint8_t Node::GetConcurrencyGroupIndex() const
{
    return 0; // Default is the unconstrained group zero
}

// CalcNameHash
//------------------------------------------------------------------------------
/*static*/ uint32_t Node::CalcNameHash( const AString & name )
{
    // xxHash3 returns a 64 bit hash and we use the lower 32 bits
    AStackString nameLower( name );
    nameLower.ToLower();
    return static_cast<uint32_t>( xxHash3::Calc64( nameLower ) );
}

// CleanMessageToPreventMSBuildFailure
//------------------------------------------------------------------------------
/*static*/ void Node::CleanMessageToPreventMSBuildFailure( const AString & msg, AString & outMsg )
{
    // Search for patterns that MSBuild detects and treats as errors:
    //
    //   <error|warning> <errorCode>: <message>
    //
    // and remove the colon so they are no longer detected:
    //
    //   <error|warning> <errorCode> <message>
    //
    // These can be anywhere in the string, and are case and whitespace insensitive
    const char * pos = msg.Get();
    for ( ;; )
    {
        const char * startPos = pos;

        // Look for error or warning
        const char * tokenPos = msg.FindI( "error ", pos );
        tokenPos = tokenPos ? tokenPos : msg.FindI( "warning ", pos );
        if ( tokenPos == nullptr )
        {
            outMsg.Append( startPos, msg.GetEnd() ); // Add rest of string
            return;
        }

        pos = tokenPos; // Advance to token

        // skip past the token
        while ( AString::IsLetter( *pos ) )
        {
            ++pos;
        }
        while ( AString::IsWhitespace( *pos ) )
        {
            ++pos;
        }

        // skip error code
        while ( AString::IsLetter( *pos ) || AString::IsNumber( *pos ) )
        {
            ++pos;
        }
        while ( AString::IsWhitespace( *pos ) )
        {
            ++pos;
        }

        // Add everything to here including the token
        outMsg.Append( startPos, pos );

        // colon?
        if ( *pos != ':' )
        {
            // Keep searching. We need to check all possible instances
            // of the token because it can appear multiple times
            continue;
        }

        // Replace colon
        outMsg += ' ';
        ++pos;
        continue; // Keep searching
    }
}

// InitializePreBuildDependencies
//------------------------------------------------------------------------------
bool Node::InitializePreBuildDependencies( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function, const Array<AString> & preBuildDependencyNames )
{
    if ( preBuildDependencyNames.IsEmpty() )
    {
        return true;
    }

    // Pre-size hint
    m_PreBuildDependencies.SetCapacity( preBuildDependencyNames.GetSize() );

    // Expand
    GetNodeListOptions options;
    options.m_AllowCopyDirNodes = true;
    options.m_AllowUnityNodes = true;
    options.m_AllowRemoveDirNodes = true;
    options.m_AllowCompilerNodes = true;
    options.m_RemoveDuplicates = true;
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".PreBuildDependencies", preBuildDependencyNames, m_PreBuildDependencies, options ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    return true;
}

// InitializeConcurrencyGroup
//------------------------------------------------------------------------------
bool Node::InitializeConcurrencyGroup( NodeGraph & nodeGraph,
                                       const BFFToken * iter,
                                       const Function * function,
                                       const AString & concurrencyGroupName,
                                       uint8_t & outConcurrencyGroupIndex )
{
    // If no ConcurrencyGroup is specified, feature is not in use
    if ( concurrencyGroupName.IsEmpty() )
    {
        return true;
    }

    // Get the ConcurrencyGroup by name
    const ConcurrencyGroup * group = nullptr;
    const SettingsNode * settings = nodeGraph.GetSettings();
    if ( settings )
    {
        group = settings->GetConcurrencyGroup( concurrencyGroupName );
    }

    // Report an error if it was not defined
    if ( group == nullptr )
    {
        Error::Error_1603_UnknownConcurrencyGroup( iter, function, concurrencyGroupName );
        return false;
    }

    // Store the inndex
    outConcurrencyGroupIndex = group->GetIndex();
    return true;
}

// GetEnvironmentString
//------------------------------------------------------------------------------
/*static*/ const char * Node::GetEnvironmentString( const Array<AString> & envVars,
                                                    const char *& inoutCachedEnvString )
{
    // Do we need a custom env string?
    if ( envVars.IsEmpty() )
    {
        // No - return build-wide environment
        return FBuild::IsValid() ? FBuild::Get().GetEnvironmentString() : nullptr;
    }

    // More than one caller could be retrieving the same env string
    // in some cases. For simplicity, we protect in all cases even
    // if we could avoid it as the mutex will not be heavily contested.
    MutexHolder mh( g_NodeEnvStringMutex );

    // If we've previously built a custom env string, use it
    if ( inoutCachedEnvString )
    {
        return inoutCachedEnvString;
    }

    // Caller owns the memory
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
