// Dependencies.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ConstMemoryStream;
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

// Dependencies
//------------------------------------------------------------------------------
class Dependencies
{
public:
    Dependencies()
    {}
    explicit Dependencies( size_t initialCapacity )
        : m_Dependencies( initialCapacity, true )
    {}
    explicit Dependencies( Dependency * otherBegin, Dependency * otherEnd )
        : m_Dependencies( otherBegin, otherEnd )
    {}

    // Index based access
    [[nodiscard]] size_t                GetSize() const { return m_Dependencies.GetSize(); }
    [[nodiscard]] bool                  IsEmpty() const { return m_Dependencies.IsEmpty(); }
    [[nodiscard]] Dependency &          operator [] ( size_t index )        { return m_Dependencies[ index ]; }
    [[nodiscard]] const Dependency &    operator [] ( size_t index ) const  { return m_Dependencies[ index ]; }
    [[nodiscard]] size_t                GetIndexOf( const Dependency * dep ) const { return m_Dependencies.GetIndexOf( dep ); }
    
    // Range based access
    [[nodiscard]] Dependency *          Begin()         { return m_Dependencies.Begin(); }
    [[nodiscard]] const Dependency *    Begin() const   { return m_Dependencies.Begin(); }
    [[nodiscard]] Dependency *          begin()         { return m_Dependencies.begin(); }
    [[nodiscard]] const Dependency *    begin() const   { return m_Dependencies.begin(); }
    [[nodiscard]] Dependency *          End()           { return m_Dependencies.End(); }
    [[nodiscard]] const Dependency *    End() const     { return m_Dependencies.End(); }
    [[nodiscard]] Dependency *          end()           { return m_Dependencies.end(); }
    [[nodiscard]] const Dependency *    end() const     { return m_Dependencies.end(); }

    // Dependency accumulation
    void                                SetCapacity( size_t capacity ) { m_Dependencies.SetCapacity( capacity); }
    void                                Clear() { m_Dependencies.Clear(); }
    void                                Add( Node * node ) { m_Dependencies.EmplaceBack( node ); }
    void                                Add( Node * node, uint64_t stamp, bool isWeak ) { m_Dependencies.EmplaceBack( node, stamp, isWeak ); }
    void                                Add( const Dependencies & deps ) { m_Dependencies.Append( deps.m_Dependencies ); }

    void Save( IOStream & stream ) const;
    void Load( NodeGraph & nodeGraph, ConstMemoryStream & stream );

protected:
    Array<Dependency> m_Dependencies;
};

//------------------------------------------------------------------------------
