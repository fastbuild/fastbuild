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
    Dependencies() = default;
    explicit Dependencies( size_t initialCapacity );
    explicit Dependencies( const Dependencies & dependencies );
    ~Dependencies();

    // Index based access
    [[nodiscard]] size_t                GetSize() const { return m_DependencyList ? m_DependencyList->m_Size : 0; }
    [[nodiscard]] size_t                GetCapacity() const { return m_DependencyList ? m_DependencyList->m_Capacity : 0; }
    [[nodiscard]] bool                  IsEmpty() const { return ( GetSize() == 0 ); }
    [[nodiscard]] Dependency &          operator [] ( size_t index );
    [[nodiscard]] const Dependency &    operator [] ( size_t index ) const;
    [[nodiscard]] size_t                GetIndexOf( const Dependency * dep ) const;

    // Range based access
    [[nodiscard]] Dependency *          Begin()         { return m_DependencyList ? GetDependencies( m_DependencyList ) : nullptr; }
    [[nodiscard]] const Dependency *    Begin() const   { return m_DependencyList ? GetDependencies( m_DependencyList ) : nullptr; }
    [[nodiscard]] Dependency *          begin()         { return m_DependencyList ? GetDependencies( m_DependencyList ) : nullptr; }
    [[nodiscard]] const Dependency *    begin() const   { return m_DependencyList ? GetDependencies( m_DependencyList ) : nullptr; }
    [[nodiscard]] Dependency *          End()           { return m_DependencyList ? GetDependencies( m_DependencyList ) + m_DependencyList->m_Size : nullptr; }
    [[nodiscard]] const Dependency *    End() const     { return m_DependencyList ? GetDependencies( m_DependencyList ) + m_DependencyList->m_Size : nullptr; }
    [[nodiscard]] Dependency *          end()           { return m_DependencyList ? GetDependencies( m_DependencyList ) + m_DependencyList->m_Size : nullptr; }
    [[nodiscard]] const Dependency *    end() const     { return m_DependencyList ? GetDependencies( m_DependencyList ) + m_DependencyList->m_Size : nullptr; }

    // Dependency accumulation
    void                                SetCapacity( size_t capacity ) { GrowCapacity( capacity ); }
    void                                Clear();
    void                                Add( Node * node );
    void                                Add( Node * node, uint64_t stamp, bool isWeak );
    void                                Add( const Dependencies & deps );
    Dependencies &                      operator = ( const Dependencies & other );

    void Save( IOStream & stream ) const;
    void Load( NodeGraph & nodeGraph, ConstMemoryStream & stream );

protected:
    // Extend to explicit capacity, or with amortized expansion if 0
    void                                GrowCapacity( size_t newCapacity  = 0 );

    // The array of dependencies and list management variables are allocated in
    // a contiguous block to save memory vs using a standard Array
    class DependencyList
    {
    public:
        uint32_t    m_Size;
        uint32_t    m_Capacity;

        // Dependencies immediately follow Size & Capacity
    };
    static Dependency *                 GetDependencies( DependencyList * depList);
    static const Dependency *           GetDependencies( const DependencyList * depList );

    DependencyList* m_DependencyList = nullptr;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
inline Dependencies::Dependencies( size_t initialCapacity )
{
    SetCapacity( initialCapacity );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
inline Dependencies::Dependencies( const Dependencies & dependencies )
{
    Add( dependencies );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
inline Dependencies::~Dependencies()
{
    FREE( m_DependencyList ); // NOTE: Skipping destruction of POD Dependency
}

// operator []
//------------------------------------------------------------------------------
inline Dependency & Dependencies::operator [] ( size_t index )
{
    ASSERT( index < GetSize() );
    return GetDependencies( m_DependencyList )[ index ];
}

// operator []
//------------------------------------------------------------------------------
inline const Dependency & Dependencies::operator [] ( size_t index ) const
{
    ASSERT( index < GetSize() );
    return GetDependencies( m_DependencyList )[ index ];
}

// GetIndexOf
//------------------------------------------------------------------------------
inline size_t Dependencies::GetIndexOf( const Dependency * dep ) const
{
    const size_t index = static_cast<size_t>( dep - GetDependencies( m_DependencyList ) );
    ASSERT( index < GetSize() );
    return index;
}

// Clear
//------------------------------------------------------------------------------
inline void Dependencies::Clear()
{
    // If we have storage, keep it but clear it
    if ( m_DependencyList )
    {
        m_DependencyList->m_Size = 0; // NOTE: Skipping destruction of POD Dependency
    }
}

// Add
//------------------------------------------------------------------------------
inline void Dependencies::Add( Node * node )
{
    if ( GetSize() == GetCapacity() )
    {
        GrowCapacity(); // Ammortized capacity growth
    }
    Dependency * newDep = &GetDependencies( m_DependencyList )[ m_DependencyList->m_Size++ ];
    INPLACE_NEW (newDep) Dependency( node );
}

// Add
//------------------------------------------------------------------------------
inline void Dependencies::Add( Node * node, uint64_t stamp, bool isWeak )
{
    if ( GetSize() == GetCapacity() )
    {
        GrowCapacity(); // Ammortized capacity growth
    }
    Dependency * newDep = &GetDependencies( m_DependencyList )[ m_DependencyList->m_Size++ ];
    INPLACE_NEW (newDep) Dependency( node, stamp, isWeak );
}

// Add
//------------------------------------------------------------------------------
inline void Dependencies::Add( const Dependencies & deps )
{
    const size_t numDepsToAdd = deps.GetSize();
    if ( numDepsToAdd > 0 )
    {
        // Expand capacity if needed
        const size_t requiredCapacity = ( GetSize() + numDepsToAdd );
        if ( requiredCapacity > GetCapacity() )
        {
            GrowCapacity( requiredCapacity ); // Expact to exact capacity
        }

        // Add elements
        Dependency * srcPos = GetDependencies( deps.m_DependencyList );
        Dependency * dstPos = GetDependencies( m_DependencyList ) + GetSize();
        for ( size_t i = 0; i < numDepsToAdd; ++i )
        {
            INPLACE_NEW ( dstPos++ ) Dependency( *srcPos++ );
        }
        m_DependencyList->m_Size += static_cast<uint32_t>( numDepsToAdd );
    }
}

// operator =
//------------------------------------------------------------------------------
inline Dependencies & Dependencies::operator = ( const Dependencies & other )
{
    Clear();
    Add( other );
    return *this;
}

// GetDependencies
//------------------------------------------------------------------------------
/*static*/ inline Dependency * Dependencies::GetDependencies( DependencyList * depList )
{
    // Dependency array follows DependencyList structure
    ASSERT( depList );
    return reinterpret_cast<Dependency *>( depList + 1 );
}

// GetDependencies
//------------------------------------------------------------------------------
/*static*/ inline const Dependency * Dependencies::GetDependencies( const DependencyList * depList )
{
    // Dependency array follows DependencyList structure
    ASSERT( depList );
    return reinterpret_cast<const Dependency *>( depList + 1 );
}

//------------------------------------------------------------------------------
