// Dependencies.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// FBuild
#include "Dependencies.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// Core
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/IOStream.h"

//------------------------------------------------------------------------------
namespace
{
#pragma pack( push, 1 )
    class SerializedDependency
    {
    public:
        uint32_t m_Index;
        uint64_t m_Stamp;
        bool m_IsWeak;
    };
#pragma pack( pop )
}

// Save
//------------------------------------------------------------------------------
void Dependencies::Save( IOStream & stream ) const
{
    const size_t numDeps = GetSize();
    if ( numDeps == 0 )
    {
        return;
    }

    SerializedDependency serializedDep;

    const Dependency * deps = GetDependencies( m_DependencyList );
    for ( size_t i = 0; i < numDeps; ++i )
    {
        const Dependency & dep = deps[ i ];

        // Prepare structure to serialize and write in one operation
        // to greatly reduce serialization overhead
        serializedDep.m_Index = dep.GetNode()->GetBuildPassTag();
        serializedDep.m_Stamp = dep.GetNodeStamp();
        serializedDep.m_IsWeak = dep.IsWeak();

        VERIFY( stream.WriteBuffer( &serializedDep, sizeof( SerializedDependency ) ) == sizeof( SerializedDependency ) );
    }
}

// Load
//------------------------------------------------------------------------------
void Dependencies::Load( NodeGraph & nodeGraph, uint32_t numDeps, ConstMemoryStream & stream )
{
    ASSERT( IsEmpty() );

    if ( numDeps == 0 )
    {
        return;
    }

    // Bypass serialization functions and directly interpret in-memory buffer
    // to avoid non-trivial function overheads
    const uint64_t pos = stream.Tell();
    const char * data = ( static_cast<const char *>( stream.GetData() ) + pos );
    stream.Seek( pos + ( sizeof( SerializedDependency ) * numDeps ) );

    SetCapacity( numDeps );
    for ( uint32_t i = 0; i < numDeps; ++i )
    {
        const SerializedDependency * dep = reinterpret_cast<const SerializedDependency *>( data ) + i;

        // Convert to Node *
        Node * node = nodeGraph.GetNodeByIndex( dep->m_Index );
        ASSERT( node );

        // Recombine dependency info
        Add( node, dep->m_Stamp, dep->m_IsWeak );
    }
}

// GrowCapacity
//------------------------------------------------------------------------------
void Dependencies::GrowCapacity( size_t newCapacity )
{
    // Expand by doubling but ensure there is always some capacity
    if ( newCapacity == 0 )
    {
        newCapacity = m_DependencyList ? ( m_DependencyList->m_Capacity * 2 )
                                       : 1;
        ASSERT( newCapacity > 0 );
    }

    // Allocate space for new list
    const size_t allocSize = ( sizeof( DependencyList ) + ( newCapacity * sizeof( Dependency ) ) );
    DependencyList * newList = static_cast<DependencyList *>( ALLOC( allocSize ) );
    newList->m_Size = 0;
    newList->m_Capacity = static_cast<uint32_t>( newCapacity );

    // Transfer old list if there is one
    if ( m_DependencyList )
    {
        // Copy construct
        const size_t numDeps = m_DependencyList->m_Size;
        Dependency * oldPos = GetDependencies( m_DependencyList );
        Dependency * newPos = GetDependencies( newList );
        for ( size_t i = 0; i < numDeps; ++i )
        {
            INPLACE_NEW ( newPos++ ) Dependency( *oldPos++ );
        }
        newList->m_Size = static_cast<uint32_t>( numDeps );

        // Free old list
        FREE( m_DependencyList ); // NOTE: Skipping destruction of POD Dependency
    }

    // Keep new list
    m_DependencyList = newList;
}

//------------------------------------------------------------------------------
