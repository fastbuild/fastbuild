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
bool Dependencies::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    ASSERT( IsEmpty() );

    uint32_t numDeps;
    if ( stream.Read( numDeps ) == false )
    {
        return false;
    }
    SetCapacity( numDeps );
    for ( uint32_t i=0; i<numDeps; ++i )
    {
        // Read node index
        uint32_t index( INVALID_NODE_INDEX );
        if ( stream.Read( index ) == false )
        {
            return false;
        }

        // Convert to Node *
        Node * node = nodeGraph.GetNodeByIndex( index );
        ASSERT( node );

        // Read Stamp
        uint64_t stamp;
        if ( stream.Read( stamp ) == false )
        {
            return false;
        }

        // Read weak flag
        bool isWeak( false );
        if ( stream.Read( isWeak ) == false )
        {
            return false;
        }

        // Recombine dependency info
        EmplaceBack( node, stamp, isWeak );
    }
    return true;
}
//------------------------------------------------------------------------------
