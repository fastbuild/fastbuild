// Array.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Forward.h"
#include "Core/Containers/Move.h"
#include "Core/Containers/Sort.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/Mem.h"

// Array
//------------------------------------------------------------------------------
template < class T >
class Array
{
public:
    explicit Array();
    explicit Array( const Array< T > & other );
    explicit Array( Array< T > && other );
    explicit Array( const T * otherBegin, const T * otherEnd );
    explicit Array( size_t initialCapacity, bool resizeable = false );
    ~Array();

    void Destruct();

    // iterators and access
    typedef T *         Iter;
    typedef const T *   ConstIter;
    Iter        Begin() const   { return m_Begin; }
    Iter        End() const     { return m_Begin + m_Size; }
    inline T &          operator [] ( size_t index )        { ASSERT( index < m_Size ); return m_Begin[ index ]; }
    inline const T &    operator [] ( size_t index ) const  { ASSERT( index < m_Size ); return m_Begin[ index ]; }
    inline T &          Top()       { ASSERT( m_Size ); return *( m_Begin + m_Size - 1 ); }
    inline const T &    Top() const { ASSERT( m_Size ); return *( m_Begin + m_Size - 1 ); }

    // C++11 style for range based for
    Iter        begin()         { return m_Begin; }
    ConstIter   begin() const   { return m_Begin; }
    ConstIter   cbegin() const  { return m_Begin; }
    Iter        end()           { return m_Begin + m_Size; }
    ConstIter   end() const     { return m_Begin + m_Size; }
    ConstIter   cend() const    { return m_Begin + m_Size; }

    // modify capacity/size
    void SetCapacity( size_t capacity );
    void SetSize( size_t size );
    void Clear();
    void Swap( Array< T > & other );

    // sorting
    void Sort() { ShellSort( m_Begin, m_Begin + m_Size, AscendingCompare() ); }
    void SortDeref() { ShellSort( m_Begin, m_Begin + m_Size, AscendingCompareDeref() ); }
    template < class COMPARER >
    void Sort( const COMPARER & comp ) { ShellSort( m_Begin, m_Begin + m_Size, comp ); }

    // find
    template < class U >
    T * Find( const U & obj ) const;
    template < class U >
    T * FindDeref( const U & obj ) const;

    // find and erase
    template < class U >
    bool FindAndErase( const U & obj );
    template < class U >
    bool FindDerefAndErase( const U & obj );

    // add/remove items
    void Append( const T & item );
    void Append( T && item );
    template < class U >
    void Append( const Array< U > & other );
    template < class U >
    void Append( const U * otherBegin, const U * otherEnd );
    void Pop();
    void PopFront(); // expensive - shuffles everything in the array!
    void Erase( T * const iter );
    inline void EraseIndex( size_t index ) { Erase( m_Begin + index ); }
    template < class ... ARGS >
    T & EmplaceBack( ARGS && ... args );

    Array & operator = ( const Array< T > & other );
    Array & operator = ( Array< T > && other );

    // query state
    inline bool     IsAtCapacity() const    { return ( m_Size == ( m_CapacityAndFlags & CAPACITY_MASK ) ); }
    inline size_t   GetCapacity() const     { return ( m_CapacityAndFlags & CAPACITY_MASK ); }
    inline size_t   GetSize() const         { return m_Size; }
    inline bool     IsEmpty() const         { return ( m_Size == 0 ); }

protected:
    void Grow();
    inline T * Allocate( size_t numElements ) const;
    inline void Deallocate( T * ptr ) const;

    // High bit of Capacity is set when memory should not be freed
    // (allocated on the stack for example)
    enum : uint32_t
    {
        DO_NOT_FREE_MEMORY_FLAG = 0x80000000,
        CAPACITY_MASK = 0x7FFFFFFF,
    };

    T *         m_Begin;
    uint32_t    m_Size;
    uint32_t    m_CapacityAndFlags;
    #if defined( ASSERTS_ENABLED )
        bool    m_Resizeable;
    #endif
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array()
    : m_Begin( nullptr )
    , m_Size( 0 )
    , m_CapacityAndFlags( 0 )
    #if defined( ASSERTS_ENABLED )
        , m_Resizeable( true )
    #endif
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array( const Array< T > & other )
    : m_Begin( nullptr )
    , m_Size( 0 )
    , m_CapacityAndFlags( 0 )
    #if defined( ASSERTS_ENABLED )
        , m_Resizeable( true )
    #endif
{
    *this = other;
}

// CONSTRUCTOR (&&)
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array( Array< T > && other )
{
    #if defined( ASSERTS_ENABLED )
        m_Resizeable = true;
    #endif

    // If memory cannot be freed it cannot be moved
    if ( other.m_CapacityAndFlags & DO_NOT_FREE_MEMORY_FLAG )
    {
        // Copy
        m_Begin = nullptr;
        m_Size = 0;
        m_CapacityAndFlags = 0;
        operator = ( Move( other ) );
    }
    else
    {
        // Move
        m_Begin = other.m_Begin;
        m_Size = other.m_Size;
        m_CapacityAndFlags = other.m_CapacityAndFlags;

        // Clear other
        other.m_Begin = nullptr;
        other.m_Size = 0;
        other.m_CapacityAndFlags = 0;
    }
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array( const T * otherBegin, const T * otherEnd )
    : Array<T>( (size_t)( otherEnd - otherBegin ) )
{
    Append( otherBegin, otherEnd );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array( size_t initialCapacity, bool resizeable )
{
    if ( initialCapacity )
    {
        #if defined( ASSERTS_ENABLED )
            m_Resizeable = true; // allow initial allocation
        #endif
        m_Begin = Allocate( initialCapacity );
        m_Size = 0;
        m_CapacityAndFlags = (uint32_t)initialCapacity;
    }
    else
    {
        m_Begin = nullptr;
        m_Size = 0;
        m_CapacityAndFlags = 0;
    }
    #if defined( ASSERTS_ENABLED )
        m_Resizeable = resizeable;
    #else
        (void)resizeable;
    #endif
}

// DESTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::~Array()
{
    T * iter = m_Begin;
    T * endIter = m_Begin + m_Size;
    while ( iter < endIter )
    {
        iter->~T();
        iter++;
    }
    Deallocate( m_Begin );
}

// Destruct
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Destruct()
{
    T * iter = m_Begin;
    T * endIter = m_Begin + m_Size;
    while ( iter < endIter )
    {
        iter->~T();
        iter++;
    }
    Deallocate( m_Begin );
    m_Begin = nullptr;
    m_Size = 0;
    m_CapacityAndFlags = 0;
}

// SetCapacity
//------------------------------------------------------------------------------
template < class T >
void Array< T >::SetCapacity( size_t capacity )
{
    if ( capacity <= GetCapacity() )
    {
        return;
    }

    T * newMem = Allocate( capacity );

    // transfer and items across and destroy all the originals
    T * src = m_Begin;
    T * endIter = src + m_Size;
    T * dst = newMem;
    while ( src < endIter )
    {
        INPLACE_NEW ( dst ) T( Move( *src ) );
        src->~T();
        src++;
        dst++;
    }

    // free old memory
    Deallocate( m_Begin );

    // hook up to new memory
    m_Begin = newMem;
    m_CapacityAndFlags = (uint32_t)capacity;
}

// SetSize
//------------------------------------------------------------------------------
template < class T >
void Array< T >::SetSize( size_t size )
{
    const size_t oldSize = m_Size;

    // no change
    if ( oldSize == size )
    {
        return;
    }

    // shrink
    if ( size < oldSize )
    {
        // destroy excess items
        T * item = m_Begin + size;
        T * endItem = m_Begin + m_Size;
        while ( item < endItem )
        {
            item->~T();
            item++;
        }
        m_Size = (uint32_t)size;
        return;
    }

    // grow

    // ensure there is enough capacity
    if ( size > GetCapacity() )
    {
        SetCapacity( size );
    }

    // create additional new items
    T * item = m_Begin + m_Size;
    T * newEnd = m_Begin + size;
    while( item < newEnd )
    {
        INPLACE_NEW ( item ) T;
        item++;
    }
    m_Size = (uint32_t)size;
}

// Clear
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Clear()
{
    // destroy all items
    T * src = m_Begin;
    T * endIter = src + m_Size;
    while ( src < endIter )
    {
        src->~T();
        src++;
    }

    // set to empty, but do not free memory
    m_Size = 0;
}

// Find
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Swap( Array< T > & other )
{
    // Neither array can be on the stack
    ASSERT( ( m_CapacityAndFlags & DO_NOT_FREE_MEMORY_FLAG ) == 0 );
    ASSERT( ( other.m_CapacityAndFlags & DO_NOT_FREE_MEMORY_FLAG ) == 0 );

    T * tmpBegin = m_Begin;
    const uint32_t tmpSize = m_Size;
    const uint32_t tmpCapacityAndFlags = m_CapacityAndFlags;
    #if defined( ASSERTS_ENABLED )
        const bool tmpResizeable = m_Resizeable;
    #endif
    m_Begin = other.m_Begin;
    m_Size = other.m_Size;
    m_CapacityAndFlags = other.m_CapacityAndFlags;
    #if defined( ASSERTS_ENABLED )
        m_Resizeable = other.m_Resizeable;
    #endif
    other.m_Begin = tmpBegin;
    other.m_Size = tmpSize;
    other.m_CapacityAndFlags = tmpCapacityAndFlags;
    #if defined( ASSERTS_ENABLED )
        other.m_Resizeable = tmpResizeable;
    #endif
}

// Find
//------------------------------------------------------------------------------
template < class T >
template < class U >
T * Array< T >::Find( const U & obj ) const
{
    T * pos = m_Begin;
    T * endPos = pos + m_Size;
    while ( pos < endPos )
    {
        if ( *pos == obj )
        {
            return pos;
        }
        pos++;
    }
    return nullptr;
}

// FindDeref
//------------------------------------------------------------------------------
template < class T >
template < class U >
T * Array< T >::FindDeref( const U & obj ) const
{
    T * pos = m_Begin;
    T * endPos = pos + m_Size;
    while ( pos < endPos )
    {
        if ( *(*pos) == obj )
        {
            return pos;
        }
        pos++;
    }
    return nullptr;
}

// FindAndErase
//------------------------------------------------------------------------------
template < class T >
template < class U >
bool Array< T >::FindAndErase( const U & obj )
{
    T * iter = Find( obj );
    if ( iter )
    {
        Erase( iter );
        return true;
    }
    return false;
}

// FindAndEraseDeref
//------------------------------------------------------------------------------
template < class T >
template < class U >
bool Array< T >::FindDerefAndErase( const U & obj )
{
    T * iter = FindDeref( obj );
    if ( iter )
    {
        Erase( iter );
        return true;
    }
    return false;
}

// Append
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Append( const T & item )
{
    if ( m_Size == ( m_CapacityAndFlags & CAPACITY_MASK ) )
    {
        Grow();
    }
    T * pos = m_Begin + m_Size;
    INPLACE_NEW ( pos ) T( item );
    m_Size++;
}

// Append
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Append( T && item )
{
    if ( m_Size == ( m_CapacityAndFlags & CAPACITY_MASK ) )
    {
        Grow();
    }
    T * pos = m_Begin + m_Size;
    INPLACE_NEW ( pos ) T( Move( item ) );
    m_Size++;
}

// Append
//------------------------------------------------------------------------------
template < class T >
template < class U >
void Array< T >::Append( const Array< U > & other )
{
    U* endPos = other.End();
    for ( U* it = other.Begin(); it != endPos; ++it )
    {
        Append( *it );
    }
}

// Append
//------------------------------------------------------------------------------
template < class T >
template < class U >
void Array< T >::Append( const U * otherBegin, const U * otherEnd )
{
    for ( const U* it = otherBegin; it != otherEnd; ++it )
    {
        Append( *it );
    }
}

// Pop
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Pop()
{
    ASSERT( m_Size ); // something must be in the array

    --m_Size;
    T * it = m_Begin + m_Size;
    it->~T();
    (void)it; // avoid warning for arrays of pod types (like uint32_t)
}

// PopFront
//------------------------------------------------------------------------------
template < class T >
void Array< T >::PopFront()
{
    ASSERT( m_Size ); // something must be in the array

    // shuffle everything backwards 1 element, overwriting the top elem
    T * dst = m_Begin;
    T * src = m_Begin + 1;
    T * endIter = m_Begin + m_Size;
    while ( src < endIter )
    {
        *dst = Move( *src );
        dst++;
        src++;
    }

    // free last element (which is now a dupe)
    dst->~T();

    --m_Size;
}

// Erase (iter)
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Erase( T * const iter )
{
    ASSERT( iter < ( m_Begin + m_Size ) );

    T * dst = iter;
    T * endIter = m_Begin + m_Size;
    T * last = (endIter - 1 );
    while ( dst < last )
    {
        *dst = Move( *( dst + 1 ) );
        dst++;
    }
    dst->~T();
    --m_Size;
}

// EmplaceBack
//------------------------------------------------------------------------------
template < class T >
template < class ... ARGS >
T & Array< T >::EmplaceBack( ARGS && ... args )
{
    if ( m_Size == ( m_CapacityAndFlags & CAPACITY_MASK ) )
    {
        Grow();
    }
    T * pos = m_Begin + m_Size;
    INPLACE_NEW ( pos ) T( Forward( ARGS, args ) ... );
    m_Size++;
    return *pos;
}

// operator =
//------------------------------------------------------------------------------
template < class T >
Array< T > & Array< T >::operator = ( const Array< T > & other )
{
    ASSERT( &other != this ); // Invalid to assign to self

    Clear();

    // need to reallocate?
    const size_t otherSize = other.GetSize();
    if ( GetCapacity() < otherSize )
    {
        Deallocate( m_Begin );
        m_Begin = Allocate( otherSize );
        m_CapacityAndFlags = (uint32_t)otherSize;
    }

    m_Size = (uint32_t)otherSize;
    T * dst = m_Begin;
    const T * endPos = dst + otherSize;
    T * src = other.m_Begin;
    while ( dst < endPos )
    {
        INPLACE_NEW ( dst ) T( *src );
        dst++;
        src++;
    }

    return *this;
}

// operator = (&&)
//------------------------------------------------------------------------------
template < class T >
Array< T > & Array< T >::operator = ( Array< T > && other )
{
    ASSERT( &other != this ); // Invalid to assign to self

    // Destruct existing elements
    Clear();

    // If memory cannot be freed it cannot be moved
    if ( other.m_CapacityAndFlags & DO_NOT_FREE_MEMORY_FLAG )
    {
        // Cannot move array, but can move elements

        // need to reallocate storage?
        const uint32_t otherSize = (uint32_t)other.GetSize();
        if ( GetCapacity() < otherSize )
        {
            Deallocate( m_Begin );
            m_Begin = Allocate( otherSize );
            m_CapacityAndFlags = (uint32_t)otherSize;
        }

        // Move elements
        T * src = other.m_Begin;
        const T * const srcEnd = other.m_Begin + other.m_Size;
        T * dst = m_Begin;
        while ( src < srcEnd )
        {
            INPLACE_NEW ( dst ) T( Move( *src ) );
            ++src;
            ++dst;
        }
        m_Size = otherSize;

        // Elements are moved, but they still need to be destructed
        // and all memory freed
        other.Destruct();
    }
    else
    {
        // Move
        if ( ( m_CapacityAndFlags & DO_NOT_FREE_MEMORY_FLAG ) == 0 )
        {
            Destruct(); // Free our own memory
        }
        m_Begin = other.m_Begin;
        m_Size = other.m_Size;
        m_CapacityAndFlags = other.m_CapacityAndFlags;

        // Clear other as we now own the memory
        other.m_Begin = nullptr;
        other.m_Size = 0;
        other.m_CapacityAndFlags = 0;
    }

    return *this;
}

// Grow
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Grow()
{
    ASSERT( m_Resizeable );

    // grow by 1.5 times (but at least by one)
    const size_t currentCapacity = GetCapacity();
    const size_t size = GetSize();
    const size_t newCapacity = ( currentCapacity + ( currentCapacity >> 1 ) + 1 );
    T * newMem = Allocate( newCapacity );

    T * src = m_Begin;
    T * dst = newMem;
    T * endIter = m_Begin + m_Size;
    while ( src < endIter )
    {
        INPLACE_NEW ( dst ) T( Move( *src ) );
        src->~T();
        dst++;
        src++;
    }
    Deallocate( m_Begin );
    m_Begin = newMem;
    m_Size = (uint32_t)size;
    m_CapacityAndFlags = (uint32_t)newCapacity;
}

// Allocate
//------------------------------------------------------------------------------
template < class T >
T * Array< T >::Allocate( size_t numElements ) const
{
    ASSERT( m_Resizeable );
    constexpr size_t align = __alignof( T ) > sizeof( void * ) ? __alignof( T ) : sizeof( void * );
    return static_cast< T * >( ALLOC( sizeof( T ) * numElements, align ) );
}

// Deallocate
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Deallocate( T * ptr ) const
{
    if ( ( m_CapacityAndFlags & DO_NOT_FREE_MEMORY_FLAG ) == 0 )
    {
        FREE( ptr );
    }
}

// StackArray
//------------------------------------------------------------------------------
template<class T, uint32_t RESERVED = 32>
class StackArray : public Array<T>
{
public:
    StackArray()
    {
        Array<T>::m_Begin = (T *)&m_Storage;
        Array<T>::m_CapacityAndFlags = ( RESERVED | Array<T>::DO_NOT_FREE_MEMORY_FLAG );
    }
    StackArray( const StackArray<T> & other )
    {
        Array<T>::m_Begin = (T*)& m_Storage;
        Array<T>::m_Size = 0;
        Array<T>::m_CapacityAndFlags = ( RESERVED | Array<T>::DO_NOT_FREE_MEMORY_FLAG );
        Array<T>::operator = ( Move( other ) );
    }
    StackArray( Array<T> && other )
    {
        Array<T>::m_Begin = (T*)& m_Storage;
        Array<T>::m_Size = 0;
        Array<T>::m_CapacityAndFlags = ( RESERVED | Array<T>::DO_NOT_FREE_MEMORY_FLAG );
        Array<T>::operator = ( Move( other ) );
    }
    StackArray( StackArray<T> && other )
    {
        Array<T>::m_Begin = (T*)& m_Storage;
        Array<T>::m_Size = 0;
        Array<T>::m_CapacityAndFlags = ( RESERVED | Array<T>::DO_NOT_FREE_MEMORY_FLAG );
        Array<T>::operator = ( Move( other ) );
    }

    inline void operator = ( const Array<T> & other )       { Array<T>::operator = ( other ); }
    inline void operator = ( const StackArray<T> & other )  { Array<T>::operator = ( other ); }
    inline void operator = ( Array<T> && other )            { Array<T>::operator = ( Move( other ) ); }
    inline void operator = ( StackArray<T> && other )       { Array<T>::operator = ( Move( other ) ); }
private:
    PRAGMA_DISABLE_PUSH_MSVC( 4324 ) // structure was padded due to alignment specifier
    alignas(__alignof(T)) uint8_t m_Storage[ RESERVED * sizeof( T ) ];
    PRAGMA_DISABLE_POP_MSVC // 4324
};

//------------------------------------------------------------------------------
