// Dependencies.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class IOStream;
class Node;
class NodeGraph;

// Dependency
//------------------------------------------------------------------------------
class Dependency
{
public:
    explicit Dependency( Node * node )
        : m_Node( node )
        , m_NodeStamp( 0 )
        , m_IsWeak( false )
    {}
    explicit Dependency( Node * node, uint64_t stamp, bool isWeak )
        : m_Node( node )
        , m_NodeStamp( stamp )
        , m_IsWeak( isWeak )
    {}

    inline Node * GetNode() const { return m_Node; }
    inline uint64_t GetNodeStamp() const { return m_NodeStamp; }
    inline bool IsWeak() const { return m_IsWeak; }

    inline void Stamp( uint64_t stamp ) { m_NodeStamp = stamp; }

private:
    Node * m_Node;  // Node being depended on
    uint64_t m_NodeStamp; // Stamp of node at last build
    bool m_IsWeak;  // Is node used for build ordering, but not triggering a rebuild
};

//  Dependencies
//------------------------------------------------------------------------------
class Dependencies : public Array< Dependency >
{
public:
    explicit inline Dependencies()
        : Array< Dependency >()
    {}
    explicit inline Dependencies( size_t initialCapacity, bool resizeable = false )
        : Array< Dependency >( initialCapacity, resizeable )
    {}
    explicit inline Dependencies( Dependency * otherBegin, Dependency * otherEnd )
        : Array< Dependency >( otherBegin, otherEnd )
    {}

    void Save( IOStream & stream ) const;
    bool Load( NodeGraph & nodeGraph, IOStream & stream );
};

//------------------------------------------------------------------------------
