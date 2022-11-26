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

    Iter endIt = End();
    for ( Iter it = Begin(); it != endIt; ++it )
    {
        const Dependency & dep = *it;

        // Nodes are saved by index to simplify deserialization
        const uint32_t index = dep.GetNode()->GetIndex();
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
        EmplaceBack( node, stamp, isWeak );
    }
}
//------------------------------------------------------------------------------
