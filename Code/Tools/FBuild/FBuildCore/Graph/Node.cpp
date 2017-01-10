// Node.cpp - base interface for dependency graph nodes
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeProxy.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/RemoveDirNode.h"
#include "Tools/FBuild/FBuildCore/Graph/SLNNode.h"
#include "Tools/FBuild/FBuildCore/Graph/TestNode.h"
#include "Tools/FBuild/FBuildCore/Graph/UnityNode.h"
#include "Tools/FBuild/FBuildCore/Graph/VCXProjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/XCodeProjectNode.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_Name.h"
#include "Tools/FBuild/FBuildCore/Graph/MetaData/Meta_AllowNonFile.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/CRC32.h"
#include "Core/Profile/Profile.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdio.h>

// Static Data
//------------------------------------------------------------------------------
/*static*/ const char * const Node::s_NodeTypeNames[Node::NUM_NODE_TYPES] =
{
    "Proxy",
    "Copy",
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
    "CopyDirNode",
    "SLN",
    "RemoveDirNode",
    "XCodeProj",
};

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

// Reflection
//------------------------------------------------------------------------------
REFLECT_STRUCT_BEGIN_ABSTRACT( Node, Struct, MetaNone() )
REFLECT_END( Node )

// CONSTRUCTOR
//------------------------------------------------------------------------------
Node::Node( const AString & name, Type type, uint32_t controlFlags )
    : m_State( NOT_PROCESSED )
    , m_BuildPassTag( 0 )
    , m_ControlFlags( controlFlags )
    , m_StatsFlags( 0 )
    , m_Stamp( 0 )
    , m_RecursiveCost( 0 )
    , m_Type( type )
    , m_Next( nullptr )
    , m_LastBuildTimeMs( 0 )
    , m_ProcessingTime( 0 )
    , m_ProgressAccumulator( 0 )
    , m_Index( INVALID_NODE_INDEX )
{
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
bool Node::DetermineNeedToBuild( bool forceClean ) const
{
    if ( forceClean )
    {
        return true;
    }

    // if we don't have a stamp, we are building for the first time
    // (or we're a node that is built every time)
    if ( m_Stamp == 0 )
    {
        // don't output for file nodes, which are always built
        if ( GetType() != Node::FILE_NODE )
        {
            FLOG_INFO( "Need to build '%s' (first time)", GetName().Get() );
        }
        return true;
    }

    if ( IsAFile() )
    {
        uint64_t lastWriteTime = FileIO::GetFileLastWriteTime( m_Name );

        if ( lastWriteTime == 0 )
        {
            // file is missing on disk
            FLOG_INFO( "Need to build '%s' (missing)", GetName().Get() );
            return true;
        }

        if ( lastWriteTime != m_Stamp )
        {
            // on disk file doesn't match our file
            // (modified by some external process)
            FLOG_INFO( "Need to build '%s' (externally modified - stamp = %llu, disk = %llu)", GetName().Get(), m_Stamp, lastWriteTime );
            return true;
        }
    }

    // static deps
    const Dependencies & staticDeps = GetStaticDependencies();
    for ( Dependencies::ConstIter it = staticDeps.Begin();
          it != staticDeps.End();
          it++ )
    {
        Node * n = it->GetNode();

        // ignore directories - the derived node should extract what it needs in DoDynamicDependencies
        if ( n->GetType() == Node::DIRECTORY_LIST_NODE )
        {
            continue;
        }

        // ignore unity nodes - the derived node should extract what it needs in DoDynamicDependencies
        if ( n->GetType() == Node::UNITY_NODE )
        {
            continue;
        }

        // Weak dependencies don't cause rebuilds
        if ( it->IsWeak() )
        {
            continue;
        }

        // we're about to compare stamps, so we should be a file (or a file list)
        ASSERT( n->IsAFile() || ( n->GetType() == Node::OBJECT_LIST_NODE ) );

        if ( n->GetStamp() == 0 )
        {
            // file missing - this may be ok, but node needs to build to find out
            FLOG_INFO( "Need to build '%s' (dep missing: '%s')", GetName().Get(), n->GetName().Get() );
            return true;
        }

        if ( n->GetStamp() > m_Stamp )
        {
            // file is newer than us
            FLOG_INFO( "Need to build '%s' (dep is newer: '%s' this = %llu, dep = %llu)", GetName().Get(), n->GetName().Get(), m_Stamp, n->GetStamp() );
            return true;
        }
    }

    // dynamic deps
    const Dependencies & dynamicDeps = GetDynamicDependencies();
    for ( Dependencies::ConstIter it = dynamicDeps.Begin();
          it != dynamicDeps.End();
          it++ )
    {
        Node * n = it->GetNode();

        // we're about to compare stamps, so we should be a file (or a file list)
        ASSERT( n->IsAFile() || ( n->GetType() == Node::OBJECT_LIST_NODE ) );

        // Weak dependencies don't cause rebuilds
        if ( it->IsWeak() )
        {
            continue;
        }

        // should be a file
        if ( n->GetStamp() == 0 )
        {
            // file missing - this may be ok, but node needs to build to find out
            FLOG_INFO( "Need to build '%s' (dep missing: '%s')", GetName().Get(), n->GetName().Get() );
            return true;
        }

        if ( n->GetStamp() > m_Stamp )
        {
            // file is newer than us
            FLOG_INFO( "Need to build '%s' (dep is newer: '%s' this = %llu, dep = %llu)", GetName().Get(), n->GetName().Get(), m_Stamp, n->GetStamp() );
            return true;
        }
    }


    // nothing needs building
    FLOG_INFO( "Up-To-Date '%s'", GetName().Get() );
    return false;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult Node::DoBuild( Job * UNUSED( job ) )
{
    ASSERT( false ); // Derived class is missing implementation
    return Node::NODE_RESULT_FAILED;
}

// DoBuild2
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult Node::DoBuild2( Job * UNUSED( job ), bool UNUSED( racingRemoteJob ) )
{
    ASSERT( false ); // Derived class is missing implementation
    return Node::NODE_RESULT_FAILED;
}

// Finalize
//------------------------------------------------------------------------------
/*virtual*/ bool Node::Finalize( NodeGraph & )
{
    // most nodes have nothing to do
    return true;
}

// SaveNodeLink
//------------------------------------------------------------------------------
/*static*/ void Node::SaveNodeLink( IOStream & fileStream, const Node * node )
{
    // for null pointer, write an empty string
    if ( node == nullptr )
    {
        fileStream.Write( AString::GetEmpty() );
    }
    else
    {
        // Can only link to nodes that are:
        //  a) Dependended on
        //  b) Our parent
        // This ensures they are saved in the correct order, which this assert checks
        ASSERT( node->IsSaved() );

        // for valid nodes, write the node name
        fileStream.Write( node->GetName() );
    }
}

// LoadNodeLink
//------------------------------------------------------------------------------
/*static*/ bool Node::LoadNodeLink( NodeGraph & nodeGraph, IOStream & stream, Node * & node )
{
    // read the name of the node
    AStackString< 512 > nodeName;
    if ( stream.Read( nodeName ) == false )
    {
        node = nullptr;
        return false;
    }

    // empty name means the pointer was null, which is supported
    if ( nodeName.IsEmpty() )
    {
        node = nullptr;
        return true;
    }

    // find the node by name - this should never fail
    Node * n = nodeGraph.FindNode( nodeName );
    if ( n == nullptr )
    {
        node = nullptr;
        return false;
    }
    node = n;

    return true;
}

// LoadNodeLink (CompilerNode)
//------------------------------------------------------------------------------
/*static*/ bool Node::LoadNodeLink( NodeGraph & nodeGraph, IOStream & stream, CompilerNode * & compilerNode )
{
    Node * node;
    if ( !LoadNodeLink( nodeGraph, stream, node ) )
    {
        return false;
    }
    if ( node == nullptr )
    {
        compilerNode = nullptr;
        return true;
    }
    if ( node->GetType() != Node::COMPILER_NODE )
    {
        return false;
    }
    compilerNode = node->CastTo< CompilerNode >();
    return true;
}

// LoadNodeLink (FileNode)
//------------------------------------------------------------------------------
/*static*/ bool Node::LoadNodeLink( NodeGraph & nodeGraph, IOStream & stream, FileNode * & fileNode )
{
    Node * node;
    if ( !LoadNodeLink( nodeGraph, stream, node ) )
    {
        return false;
    }
    if ( node == nullptr )
    {
        fileNode = nullptr;
        return true;
    }
    if ( !node->IsAFile() )
    {
        return false;
    }
    fileNode = node->CastTo< FileNode >();
    return ( fileNode != nullptr );
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

// Load
//------------------------------------------------------------------------------
/*static*/ Node * Node::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    // read type
    uint32_t nodeType;
    if ( stream.Read( nodeType ) == false )
    {
        return nullptr;
    }

    PROFILE_SECTION(Node::GetTypeName((Type)nodeType));

    // read stamp (but not for file nodes)
    uint64_t stamp( 0 );
    if ( nodeType != Node::FILE_NODE )
    {
        if ( stream.Read( stamp ) == false )
        {
            return nullptr;
        }
    }

    // read contents
    Node * n = nullptr;
    switch ( (Node::Type)nodeType )
    {
        case Node::PROXY_NODE:          ASSERT( false );                                    break;
        case Node::COPY_FILE_NODE:      n = CopyFileNode::Load( nodeGraph, stream );        break;
        case Node::DIRECTORY_LIST_NODE: n = DirectoryListNode::Load( nodeGraph, stream );   break;
        case Node::EXEC_NODE:           n = ExecNode::Load( nodeGraph, stream );            break;
        case Node::FILE_NODE:           n = FileNode::Load( nodeGraph, stream );            break;
        case Node::LIBRARY_NODE:        n = LibraryNode::Load( nodeGraph, stream );         break;
        case Node::OBJECT_NODE:         n = ObjectNode::Load( nodeGraph, stream );          break;
        case Node::ALIAS_NODE:          n = AliasNode::Load( nodeGraph, stream );           break;
        case Node::EXE_NODE:            n = ExeNode::Load( nodeGraph, stream );             break;
        case Node::CS_NODE:             n = CSNode::Load( nodeGraph, stream );              break;
        case Node::UNITY_NODE:          n = UnityNode::Load( nodeGraph, stream );           break;
        case Node::TEST_NODE:           n = TestNode::Load( nodeGraph, stream );            break;
        case Node::COMPILER_NODE:       n = CompilerNode::Load( nodeGraph, stream );        break;
        case Node::DLL_NODE:            n = DLLNode::Load( nodeGraph, stream );             break;
        case Node::VCXPROJECT_NODE:     n = VCXProjectNode::Load( nodeGraph, stream );      break;
        case Node::OBJECT_LIST_NODE:    n = ObjectListNode::Load( nodeGraph, stream );      break;
        case Node::COPY_DIR_NODE:       n = CopyDirNode::Load( nodeGraph, stream );         break;
        case Node::SLN_NODE:            n = SLNNode::Load( nodeGraph, stream );             break;
        case Node::REMOVE_DIR_NODE:     n = RemoveDirNode::Load( nodeGraph, stream );       break;
        case Node::XCODEPROJECT_NODE:   n = XCodeProjectNode::Load( nodeGraph, stream );    break;
        case Node::NUM_NODE_TYPES:      ASSERT( false );                        break;
    }

    ASSERT( n );
    if ( n )
    {
        // set stamp
        n->m_Stamp = stamp;
    }

    return n;
}

//
//------------------------------------------------------------------------------
/*static*/ void Node::Save( IOStream & stream, const Node * node )
{
    ASSERT( node );

    // save type
    uint32_t nodeType = (uint32_t)node->GetType();
    stream.Write( nodeType );

    // save stamp (but not for file nodes)
    if ( nodeType != Node::FILE_NODE )
    {
        uint64_t stamp = node->GetStamp();
        stream.Write( stamp );
    }

    // save contents
    node->Save( stream );
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
/*virtual*/ void Node::SaveRemote( IOStream & UNUSED( stream ) ) const
{
    // Should never get here.  Either:
    // a) Derived Node is missing SaveRemote implementation
    // b) Serializing an unexpected type
    ASSERT( false );
}

// Serialize
//------------------------------------------------------------------------------
void Node::Serialize( IOStream & stream ) const
{
    // Deps
    NODE_SAVE_DEPS( m_PreBuildDependencies );
    NODE_SAVE_DEPS( m_StaticDependencies );
    NODE_SAVE_DEPS( m_DynamicDependencies );

    // Properties
    const ReflectionInfo * const ri = GetReflectionInfoV();
    Serialize( stream, this, *ri );

    #if defined( DEBUG )
        MarkAsSaved();
    #endif
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
                Array< AString > * arrayOfStrings( nullptr );
                property.GetPtrToProperty( base, arrayOfStrings );
                VERIFY( stream.Write( *arrayOfStrings ) );
            }
            else
            {
                AString * string( nullptr );
                property.GetPtrToProperty( base, string );
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
            if ( property.IsArray() )
            {
                // Write number of elements
                const auto & propertyS = static_cast< const ReflectedPropertyStruct & >( property );
                const uint32_t numElements = (uint32_t)propertyS.GetArraySize( base );
                VERIFY( stream.Write( numElements ) );

                // Write each element
                for ( uint32_t i=0; i<numElements; ++i )
                {
                    const void * structBase = propertyS.GetStructInArray( base, (size_t)i );
                    Serialize( stream, structBase, *propertyS.GetStructReflectionInfo() );
                }
                return;
            }
            break; // Fall through to error
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
bool Node::Deserialize( NodeGraph & nodeGraph, IOStream & stream )
{
    // Deps
    NODE_LOAD_DEPS( 0,          preBuildDeps );
    ASSERT( m_PreBuildDependencies.IsEmpty() );
    m_PreBuildDependencies.Append( preBuildDeps );
    NODE_LOAD_DEPS( 0,          staticDeps );
    ASSERT( m_StaticDependencies.IsEmpty() );
    m_StaticDependencies.Append( staticDeps );
    NODE_LOAD_DEPS( 0,          dynamicDeps );
    ASSERT( m_DynamicDependencies.IsEmpty() );
    m_DynamicDependencies.Append( dynamicDeps );

    // Properties
    const ReflectionInfo * const ri = GetReflectionInfoV();
    return Deserialize( stream, this, *ri );
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
                Array< AString > * arrayOfStrings( nullptr );
                property.GetPtrToProperty( base, arrayOfStrings );
                if ( stream.Read( *arrayOfStrings ) == false )
                {
                    return false;
                }
            }
            else
            {
                AString * string = nullptr;
                property.GetPtrToProperty( base, string );
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
            if ( property.IsArray() )
            {
                // Read number of elements
                uint32_t numElements( 0 );
                if ( stream.Read( numElements ) == false )
                {
                    return false;
                }
                const auto & propertyS = static_cast< const ReflectedPropertyStruct & >( property );
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
            break; // Fall through to error
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
                                  const char * data,
                                  uint32_t dataSize,
                                  const Array< AString > * exclusions )
{
    if ( ( data == nullptr ) || ( dataSize == 0 ) )
    {
        return;
    }

    // preallocate a large buffer
    AString buffer( MEGABYTE );

    const char * end = data + dataSize;
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
                // (FBuild is null in remote context - fixup occurs on master)
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
    // To:
    //     <path>\Core\Mem\Mem.h(23,1): warning: some warning text
    //
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

    // SNC Style
    // Convert:
    //     Core/Mem/Mem.h(23,1): warning 55: some warning text
    // To:
    //     <path>\Core\Mem\Mem.h(23,1): warning 55: some warning text
    //
    tag = tag ? tag : line.Find( ": error " );
    tag = tag ? tag : line.Find( ": warning " );
    tag = tag ? tag : line.Find( ": note " );
    tag = tag ? tag : line.Find( ": remark " );
    if ( tag )
    {
        FixupPathForVSIntegration_SNC( line, tag );
        return;
    }

    // leave line untouched
}

// FixupPathForVSIntegration_GCC
//------------------------------------------------------------------------------
/*static*/ void Node::FixupPathForVSIntegration_GCC( AString & line, const char * tag )
{
    AStackString<> beforeTag( line.Get(), tag );
    Array< AString > tokens;
    beforeTag.Tokenize( tokens, ':' );
    const size_t numTokens = tokens.GetSize();
    if ( numTokens < 3 )
    {
        return; // not enough : to be correctly formatted
    }

    // are last two tokens numbers?
    int row, column;
    if ( ( sscanf( tokens[ numTokens - 1 ].Get(), "%i", &column ) != 1 ) ||
         ( sscanf( tokens[ numTokens - 2 ].Get(), "%i", &row ) != 1 ) )
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
        if ( i != 0 )
        {
            fixed += ':';
        }
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
    if( openBracket == nullptr )
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

// InitializePreBuildDependencies
//------------------------------------------------------------------------------
bool Node::InitializePreBuildDependencies( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function, const Array< AString > & preBuildDependencyNames )
{
    if ( preBuildDependencyNames.IsEmpty() )
    {
        return true;
    }

    // Pre-size hint
    m_PreBuildDependencies.SetCapacity( preBuildDependencyNames.GetSize() );

    // Expand
    for ( const AString & preDepName : preBuildDependencyNames )
    {
        if ( !Function::GetNodeList( nodeGraph, iter, function, ".PreBuildDependencies", preDepName, m_PreBuildDependencies, true, true, true ) )
        {
            return false; // GetNodeList will have emitted an error
        }
    }

    return true;
}

//------------------------------------------------------------------------------
