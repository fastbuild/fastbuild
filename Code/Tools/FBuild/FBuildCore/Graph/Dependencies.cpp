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
#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/ConstMemoryStream.h"

// Save
//------------------------------------------------------------------------------
void Dependencies::Save( IOStream & stream ) const
{
    const size_t numDeps = GetSize();
    stream.Write( (uint32_t)numDeps );
    if ( numDeps == 0 )
    {
        return;
    }

    const Dependency * deps = GetDependencies( m_DependencyList );
    for ( size_t i = 0; i < numDeps; ++i )
    {
        const Dependency & dep = deps[ i ];

        // Save index of node we depend on
        const uint32_t index = dep.GetNode()->GetBuildPassTag();
        stream.Write( index );

        // Save stamp
        const uint64_t stamp = dep.GetNodeStamp();
        stream.Write( stamp );

        // Save weak flag
        const bool isWeak = dep.IsWeak();
        stream.Write( isWeak );
    }
}

// Load
//------------------------------------------------------------------------------
void Dependencies::Load( NodeGraph & nodeGraph, ConstMemoryStream & stream )
{
    ASSERT( IsEmpty() );

    uint32_t numDeps;
    VERIFY( stream.Read( numDeps ) );
    if ( numDeps == 0 )
    {
        return;
    }
    
    SetCapacity( numDeps );
    for ( uint32_t i=0; i<numDeps; ++i )
    {
        // Read node index
        uint32_t index( INVALID_NODE_INDEX );
        VERIFY( stream.Read( index ) );

        // Convert to Node *
        Node * node = nodeGraph.GetNodeByIndex( index );
        ASSERT( node );

        // Read Stamp
        uint64_t stamp;
        VERIFY( stream.Read( stamp ) );

        // Read weak flag
        bool isWeak( false );
        VERIFY( stream.Read( isWeak ) );

        // Recombine dependency info
        Add( node, stamp, isWeak );
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
    const size_t allocSize = ( sizeof(DependencyList) + ( newCapacity * sizeof(Dependency) ) );
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
        newList->m_Size = static_cast<uint32_t>(numDeps);

        // Free old list
        FREE( m_DependencyList ); // NOTE: Skipping destruction of POD Dependency
    }

    // Keep new list
    m_DependencyList = newList;
}

//------------------------------------------------------------------------------
