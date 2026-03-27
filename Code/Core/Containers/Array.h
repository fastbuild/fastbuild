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
#include "Core/Process/Atomic.h"

#include <type_traits>

// Array
//------------------------------------------------------------------------------
template <class T>
class Array
{
    static constexpr bool s_TIsShareable = std::is_copy_constructible_v<T>;
public:
    explicit Array();
    explicit Array( const Array<T> & other );
    explicit Array( Array<T> && other );
    explicit Array( const T * otherBegin, const T * otherEnd );
    ~Array();

    void Destruct();

    // iterators and access
    using ValueType = T;
    typedef T * Iter;
    typedef const T * ConstIter;
    [[nodiscard]] ConstIter Begin() const { return m_Begin; }
    [[nodiscard]] ConstIter End() const { return m_Begin + m_Size; }
    [[nodiscard]] Iter Begin() { EnsureIsOnlyOwner(); return m_Begin; }
    [[nodiscard]] Iter End() { EnsureIsOnlyOwner(); return m_Begin + m_Size; }
    [[nodiscard]] T & operator[]( size_t index )
    {
        ASSERT( index < m_Size );
        EnsureIsOnlyOwner();
        return m_Begin[ index ];
    }
    [[nodiscard]] const T & operator[]( size_t index ) const
    {
        ASSERT( index < m_Size );
        return m_Begin[ index ];
    }
    [[nodiscard]] bool Contains( const T * it ) const;
    [[nodiscard]] size_t GetIndexOf( const T * it ) const;
    [[nodiscard]] T & Top()
    {
        ASSERT( m_Size );
        EnsureIsOnlyOwner();
        return *( m_Begin + m_Size - 1 );
    }
    [[nodiscard]] const T & Top() const
    {
        ASSERT( m_Size );
        return *( m_Begin + m_Size - 1 );
    }

    // C++11 style for range based for
    [[nodiscard]] Iter begin() { EnsureIsOnlyOwner(); return m_Begin; }
    [[nodiscard]] ConstIter begin() const { return m_Begin; }
    [[nodiscard]] ConstIter cbegin() const { return m_Begin; }
    [[nodiscard]] Iter end() { EnsureIsOnlyOwner(); return m_Begin + m_Size; }
    [[nodiscard]] ConstIter end() const { return m_Begin + m_Size; }
    [[nodiscard]] ConstIter cend() const { return m_Begin + m_Size; }

    // modify capacity/size
    void SetCapacity( size_t capacity );
    void SetSize( size_t size );
    void Clear();
    void Swap( Array<T> & other );

    // sorting
    void Sort() { EnsureIsOnlyOwner(); ShellSort( m_Begin, m_Begin + m_Size, AscendingCompare() ); }
    void SortDeref() { EnsureIsOnlyOwner(); ShellSort( m_Begin, m_Begin + m_Size, AscendingCompareDeref() ); }
    template <class COMPARER>
    void Sort( const COMPARER & comp )
    {
        EnsureIsOnlyOwner();
        ShellSort( m_Begin, m_Begin + m_Size, comp );
    }

    // find
    template <class U>
    [[nodiscard]] const T * Find( const U & obj ) const;
    template <class U>
    [[nodiscard]] T * Find( const U & obj ) { return ToMutableContentPtr( const_cast<const Array<T> *>( this )->Find( obj ) ); }
    template <class U>
    [[nodiscard]] const T * FindDeref( const U & obj ) const;
    template <class U>
    [[nodiscard]] T * FindDeref( const U & obj ) { return ToMutableContentPtr( const_cast<const Array<T> *>( this )->FindDeref( obj ) ); }

    // find and erase
    template <class U>
    bool FindAndErase( const U & obj );
    template <class U>
    bool FindDerefAndErase( const U & obj );
    template <class U>
    size_t FindAndEraseAll( const U & obj );

    // add/remove items
    void Append( const T & item );
    void Append( T && item );
    template <class U>
    void Append( const Array<U> & other );
    template <class U>
    void Append( Array<U> && other );
    template <class U>
    void Append( const U * otherBegin, const U * otherEnd );
private:
    enum class Strategy
    {
        Copy,
        Move,
    };
    template <class U, Strategy S>
    void AppendInternal( U * otherBegin, U * otherEnd );
public:
    void Pop();
    void PopFront(); // expensive - shuffles everything in the array!
    void Erase( const T * const iter );
    void EraseIndex( size_t index ) { Erase( m_Begin + index ); }
    template <class... ARGS>
    T & EmplaceBack( ARGS &&... args );

    Array & operator=( const Array<T> & other );
    Array & operator=( Array<T> && other );

    // query state
    [[nodiscard]] bool IsAtCapacity() const { return ( m_Size == m_Capacity ); }
    [[nodiscard]] size_t GetCapacity() const { return m_Capacity; }
    [[nodiscard]] size_t GetSize() const { return m_Size; }
    [[nodiscard]] bool IsEmpty() const { return ( m_Size == 0 ); }

private:
    T * ToMutableContentPtr( const T * ptr );
    void EnsureIsOnlyOwner();
    void EnsureIsOnlyOwner( size_t capacity );
    void CopyToSharedData( size_t capacity );

    static constexpr size_t s_Align = Math::Max( __alignof( T ), sizeof( void * ) );
    static constexpr size_t s_BeginOffset = s_TIsShareable ? Math::RoundUp( sizeof( Atomic<uint32_t> ), Array<T>::s_Align ) : 0;

protected:
    bool IsOnlyOwner() const
    {
        if constexpr ( !s_TIsShareable )
        {
            return true;
        }
        return ( m_ReferenceCount == nullptr ) || ( m_ReferenceCount->Load() == 1 );
    }
    bool IsUsingSharedMemory() const
    {
        if constexpr ( !s_TIsShareable )
        {
            return false;
        }
        return ( m_ReferenceCount != nullptr ) || ( m_Begin == nullptr );
    }

    void Grow();
    void Grow( size_t minimumSize );
    static void Allocate( Atomic<uint32_t> * & referenceCount, T * & begin, size_t numElements );
    static void Release( Atomic<uint32_t> * referenceCount, const T * begin, const T * end );
    static void Delete( const T * begin, const T * end );

    Atomic<uint32_t> * m_ReferenceCount;
    T * m_Begin;
    uint32_t m_Size;
    uint32_t m_Capacity;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <class T>
Array<T>::Array()
    : m_ReferenceCount( nullptr )
    , m_Begin( nullptr )
    , m_Size( 0 )
    , m_Capacity( 0 )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <class T>
Array<T>::Array( const Array<T> & other )
    : Array()
{
    *this = other;
}

// CONSTRUCTOR (&&)
//------------------------------------------------------------------------------
template <class T>
Array<T>::Array( Array<T> && other )
    : Array()
{
    *this = Move( other );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <class T>
Array<T>::Array( const T * otherBegin, const T * otherEnd )
    : Array()
{
    Append( otherBegin, otherEnd );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
template <class T>
Array<T>::~Array()
{
    Array<T>::Release( m_ReferenceCount, m_Begin, m_Begin + m_Size );
}

// Destruct
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Destruct()
{
    Array<T>::Release( m_ReferenceCount, m_Begin, m_Begin + m_Size );
    if ( m_ReferenceCount )
    {
        m_ReferenceCount = nullptr;
        m_Begin = nullptr;
    }
    m_Size = 0;
    m_Capacity = 0;
}

// Contains
//------------------------------------------------------------------------------
template <class T>
bool Array<T>::Contains( const T * it ) const
{
    const size_t index = static_cast<size_t>( it - m_Begin );
    return ( index < m_Size ); // out of bounds pointer will result in invalid index
}

// GetIndexOf
//------------------------------------------------------------------------------
template <class T>
size_t Array<T>::GetIndexOf( const T * it ) const
{
    const size_t index = static_cast<size_t>( it - m_Begin );
    ASSERT( index < m_Size ); // out of bounds pointer will result in invalid index
    return index;
}

// SetCapacity
//------------------------------------------------------------------------------
template <class T>
void Array<T>::SetCapacity( size_t capacity )
{
    if ( capacity <= GetCapacity() )
    {
        return;
    }
    CopyToSharedData( capacity );
}

// SetSize
//------------------------------------------------------------------------------
template <class T>
void Array<T>::SetSize( size_t size )
{
    // clear
    if ( size == 0 )
    {
        Clear();
        return;
    }

    const size_t oldSize = m_Size;

    // no change
    if ( oldSize == size )
    {
        return;
    }

    // shrink
    if ( size < oldSize )
    {
        if ( !IsOnlyOwner() )
        {
            // if data is shared with other arrays, create a copy excluding the excess items
            CopyToSharedData( size );
            return;
        }

        // destroy excess items
        T * item = m_Begin + size;
        T * endItem = m_Begin + m_Size;
        Array<T>::Delete( item, endItem );

        // reduce size without reducing capacity
        m_Size = (uint32_t)size;
        return;
    }

    // grow

    // ensure there is enough capacity
    SetCapacity( size );
    EnsureIsOnlyOwner();

    // create additional new items
    T * const newEnd = m_Begin + size;
    for ( T * item = m_Begin + oldSize; item < newEnd; ++item )
    {
        INPLACE_NEW( item ) T();
    }
    m_Size = (uint32_t)size;
}

// Clear
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Clear()
{
    // no change
    if ( m_Size == 0 )
    {
        return;
    }

    if ( !IsOnlyOwner() )
    {
        Array<T>::Release( m_ReferenceCount, m_Begin, m_Begin + m_Size );
        m_ReferenceCount = nullptr;
        m_Begin = nullptr;
        m_Size = 0;
        m_Capacity = 0;
        return;
    }

    // destroy all contents
    const T * const end = m_Begin + m_Size;
    for ( T * item = m_Begin; item < end; ++item )
    {
        item->~T();
    }

    // continue to hold on to the memory
    m_Size = 0;
}

// Swap
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Swap( Array<T> & other )
{
    // Neither array can be on the stack
    ASSERT( IsUsingSharedMemory() && other.IsUsingSharedMemory() );

    Atomic<uint32_t> * const tmpReferenceCount = m_ReferenceCount;
    T * const tmpBegin = m_Begin;
    const uint32_t tmpSize = m_Size;
    const uint32_t tmpCapacityAndFlags = m_Capacity;

    m_ReferenceCount = other.m_ReferenceCount;
    m_Begin = other.m_Begin;
    m_Size = other.m_Size;
    m_Capacity = other.m_Capacity;

    other.m_ReferenceCount = tmpReferenceCount;
    other.m_Begin = tmpBegin;
    other.m_Size = tmpSize;
    other.m_Capacity = tmpCapacityAndFlags;
}

// Find
//------------------------------------------------------------------------------
template <class T>
template <class U>
const T * Array<T>::Find( const U & obj ) const
{
    const T * pos = m_Begin;
    const T * endPos = pos + m_Size;
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
template <class T>
template <class U>
const T * Array<T>::FindDeref( const U & obj ) const
{
    T * pos = m_Begin;
    T * endPos = pos + m_Size;
    while ( pos < endPos )
    {
        if ( *( *pos ) == obj )
        {
            return pos;
        }
        pos++;
    }
    return nullptr;
}

// FindAndErase
//------------------------------------------------------------------------------
template <class T>
template <class U>
bool Array<T>::FindAndErase( const U & obj )
{
    const T * iter = const_cast<const Array<T> *>( this )->Find( obj );
    if ( iter )
    {
        Erase( iter );
        return true;
    }
    return false;
}

// FindAndEraseDeref
//------------------------------------------------------------------------------
template <class T>
template <class U>
bool Array<T>::FindDerefAndErase( const U & obj )
{
    const T * iter = const_cast<const Array<T> *>( this )->FindDeref( obj );
    if ( iter )
    {
        Erase( iter );
        return true;
    }
    return false;
}

// FindAndEraseAll
//------------------------------------------------------------------------------
template <class T>
template <class U>
size_t Array<T>::FindAndEraseAll( const U & obj )
{
    const T * firstMatch = Find( obj );

    if ( !firstMatch )
    {
        // avoid eager copies if nothing in the array matches
        return 0;
    }

    if constexpr ( Array<T>::s_TIsShareable )
    {
        if ( !IsOnlyOwner() )
        {
            // copy non-erased items and leave the erased items behind
            Atomic<uint32_t> * const oldReferenceCount = m_ReferenceCount;
            const T * const oldBegin = m_Begin;
            const T * const oldEnd = m_Begin + m_Size;
            const size_t oldSize = m_Size;

            // hook up to new memory
            const size_t newCapacity = oldSize - 1; // we know that at least 1 item is going to be erased.
            Array<T>::Allocate( m_ReferenceCount, m_Begin, newCapacity );
            m_Capacity = newCapacity;

            const T * src = oldBegin;
            T * dst = m_Begin;

            for ( ; src < firstMatch ; ++src, ++dst )
            {
                INPLACE_NEW( dst ) T( *src );
            }
            ++src; // avoid comparing firstMatch twice
            for ( ; src < oldEnd ; ++src )
            {
                if ( *src == obj )
                {
                    INPLACE_NEW( dst ) T( *src );
                    ++dst;
                }
            }
            m_Size = (uint32_t)( dst - m_Begin );

            // release old memory. should just decrement the reference count.
            Array<T>::Release( oldReferenceCount, oldBegin, oldEnd );
            return ( oldSize - (size_t)m_Size );
        }
    }

    // operate on the data in-place
    T * src = const_cast<T *>( firstMatch );
    T * dst = src;
    T * end = m_Begin + m_Size;

    // avoid comparing firstMatch twice by deleting immediately
    src->~T();
    ++src;

    for ( ; src < end ; ++src )
    {
        if ( *src == obj )
        {
            INPLACE_NEW( dst ) T( Move ( *src ) );
            ++dst;
        }
        src->~T();
    }

    m_Size = (uint32_t)( dst - m_Begin );
    return (size_t)( src - dst );
}

// Append
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Append( const T & item )
{
    if ( m_Size == m_Capacity )
    {
        Grow();
    }
    else
    {
        EnsureIsOnlyOwner( m_Capacity );
    }
    T * pos = m_Begin + m_Size;
    INPLACE_NEW( pos ) T( item );
    m_Size++;
}

// Append
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Append( T && item )
{
    if ( m_Size == m_Capacity )
    {
        Grow();
    }
    else
    {
        EnsureIsOnlyOwner( m_Capacity );
    }
    T * pos = m_Begin + m_Size;
    INPLACE_NEW( pos ) T( Move( item ) );
    m_Size++;
}

// Append
//------------------------------------------------------------------------------
template <class T>
template <class U>
void Array<T>::Append( const Array<U> & other )
{
    if ( IsEmpty() )
    {
        (*this) = other;
        return;
    }
    AppendInternal<const U, Strategy::Copy>( other.Begin(), other.End() );
}

// Append
//------------------------------------------------------------------------------
template <class T>
template <class U>
void Array<T>::Append( Array<U> && other )
{
    if ( IsEmpty() )
    {
        (*this) = Move( other );
        return;
    }
    AppendInternal<U, Strategy::Move>( other.Begin(), other.End() );
}

// Append
//------------------------------------------------------------------------------
template <class T>
template <class U>
void Array<T>::Append( const U * otherBegin, const U * otherEnd )
{
    AppendInternal<const U, Strategy::Copy>( otherBegin, otherEnd );
}

// Append
//------------------------------------------------------------------------------
template <class T>
template <class U, typename Array<T>::Strategy S>
void Array<T>::AppendInternal( U * otherBegin, U * otherEnd )
{
    const size_t otherSize = static_cast<size_t>( otherEnd - otherBegin );
    const size_t newSize = m_Size + otherSize;
    if ( otherSize == 0)
    {
        return; // nothing to append
    }
    if ( newSize > m_Capacity )
    {
        Grow( newSize );
    }
    else
    {
        EnsureIsOnlyOwner( m_Capacity );
    }

    T * dest = m_Begin + m_Size;
    for ( U * it = otherBegin; it != otherEnd; ++it, ++dest)
    {
        if constexpr ( S == Strategy::Copy )
        {
            INPLACE_NEW( dest ) T( *it );
        }
        else if constexpr ( S == Strategy::Move )
        {
            INPLACE_NEW( dest ) T( Move ( *it ) );
        }
        else
        {
            static_assert( sizeof(S) == 0, "Append strategy case not implemented" );
        }
    }
    m_Size = (uint32_t)newSize;
}

// Pop
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Pop()
{
    ASSERT( m_Size ); // something must be in the array

    if ( !IsOnlyOwner() )
    {
        CopyToSharedData( m_Size - 1 );
        return;
    }

    --m_Size;
    T * it = m_Begin + m_Size;
    it->~T();
    (void)it; // avoid warning for arrays of pod types (like uint32_t)
}

// PopFront
//------------------------------------------------------------------------------
template <class T>
void Array<T>::PopFront()
{
    Erase( m_Begin );
}

// Erase (iter)
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Erase( const T * const iter )
{
    ASSERT( iter >= m_Begin ); // iter must be in the array
    ASSERT( iter < ( m_Begin + m_Size ) );

    if constexpr ( Array<T>::s_TIsShareable )
    {
        if ( !IsOnlyOwner() )
        {
            Atomic<uint32_t> * const oldReferenceCount = m_ReferenceCount;
            T * const oldBegin = m_Begin;
            T * const oldEnd = m_Begin + m_Size;

            // hook up to new memory
            size_t newCapacity = m_Size - 1;
            Array<T>::Allocate( m_ReferenceCount, m_Begin, newCapacity );
            m_Capacity = newCapacity;
            m_Size = newCapacity;

            T * const newBegin = m_Begin;

            // copy remaining items in new location
            const bool doMove = ( oldReferenceCount == nullptr ) || ( oldReferenceCount->Load() == 1 );
            T * src = oldBegin;
            T * dst = newBegin;
            while (src < oldEnd )
            {
                if ( doMove )
                {
                    // this array is the last owner of the heap data, so move the items
                    INPLACE_NEW( dst ) T( Move( *src ) );
                }
                else
                {
                    // this array shares data with other arrays, so copy the items
                    INPLACE_NEW( dst ) T( *src );
                }
                src++;
                dst++;
                if ( src == iter )
                {
                    src++; // skip the item we're erasing
                }
            }

            // release old memory and maybe free and destroy the old items
            Array<T>::Release( oldReferenceCount, oldBegin, oldEnd );
            return;
        }
    }

    T * dst = const_cast<T *>( iter );
    T * endIter = m_Begin + m_Size;
    T * last = ( endIter - 1 );
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
template <class T>
template <class... ARGS>
T & Array<T>::EmplaceBack( ARGS &&... args )
{
    if ( m_Size == m_Capacity )
    {
        Grow();
    }
    else
    {
        EnsureIsOnlyOwner( m_Capacity );
    }
    T * pos = m_Begin + m_Size;
    INPLACE_NEW( pos ) T( Forward( ARGS, args )... );
    m_Size++;
    return *pos;
}

// operator =
//------------------------------------------------------------------------------
template <class T>
Array<T> & Array<T>::operator=( const Array<T> & other )
{
    if ( &other == this )
    {
        return *this;
    }

    if ( other.IsUsingSharedMemory() && IsUsingSharedMemory() && ( (void *)m_Begin == (void *)other.m_Begin ) )
    {
        // Already pointing to the same shared memory. Nothing to do.
        ASSERT( ( (void *)m_ReferenceCount == (void *)other.m_ReferenceCount ) && ( m_Size == other.m_Size ) && ( m_Capacity == other.m_Capacity ) );
        return *this;
    }

    // Use shallow copy if both are using shared memory or current should switch to shared memory
    if ( other.IsUsingSharedMemory() && ( IsUsingSharedMemory() || ( other.GetSize() > GetCapacity() ) ) )
    {
        // If both arrays are using heap-allocated data, just copy the pointers and increment the counter.
        Array<T>::Release( m_ReferenceCount, m_Begin, m_Begin + m_Size );
        m_ReferenceCount = other.m_ReferenceCount;
        m_Begin = other.m_Begin;
        m_Size = other.m_Size;
        m_Capacity = other.m_Capacity;
        if ( m_ReferenceCount )
        {
            m_ReferenceCount->Increment();
        }
        return *this;
    }

    // Destruct existing elements
    Clear();

    // need to reallocate?
    const size_t otherSize = other.GetSize();
    SetCapacity( otherSize );

    m_Size = (uint32_t)otherSize;
    T * dst = m_Begin;
    const T * endPos = dst + otherSize;
    const T * src = other.m_Begin;
    while ( dst < endPos )
    {
        INPLACE_NEW( dst ) T( *src );
        dst++;
        src++;
    }

    return *this;
}

// operator = (&&)
//------------------------------------------------------------------------------
template <class T>
Array<T> & Array<T>::operator=( Array<T> && other )
{
    ASSERT( &other != this ); // Invalid to assign to self

    if ( other.IsUsingSharedMemory() && IsUsingSharedMemory() && ( (void *)m_Begin == (void *)other.m_Begin ) )
    {
        // Already pointing to the same shared memory. Nothing to do.
        ASSERT( ( (void *)m_ReferenceCount == (void *)other.m_ReferenceCount ) && ( m_Size == other.m_Size ) && ( m_Capacity == other.m_Capacity ) );
        return *this;
    }

    // Use shallow copy if both are using shared memory or current should switch to shared memory
    if ( other.IsUsingSharedMemory() && ( IsUsingSharedMemory() || ( other.GetSize() > GetCapacity() ) ) )
    {
        // If both arrays are using heap-allocated data, just copy the pointers and increment the counter.
        Array<T>::Release( m_ReferenceCount, m_Begin, m_Begin + m_Size );
        m_ReferenceCount = other.m_ReferenceCount;
        m_Begin = other.m_Begin;
        m_Size = other.m_Size;
        m_Capacity = other.m_Capacity;
        other.m_ReferenceCount = nullptr;
        other.m_Begin = nullptr;
        other.m_Size = 0;
        other.m_Capacity = 0;
        return *this;
    }

    // Destruct existing elements
    Clear();

    // need to reallocate?
    const size_t otherSize = other.GetSize();
    SetCapacity( otherSize );

    m_Size = (uint32_t)otherSize;
    T * dst = m_Begin;
    const T * endPos = dst + otherSize;
    T * src = other.m_Begin;
    while ( dst < endPos )
    {
        INPLACE_NEW( dst ) T( Move( *src ) );
        dst++;
        src++;
    }

    other.Clear();
    return *this;
}

// ToMutableContentPtr (const T *)
//------------------------------------------------------------------------------
template <class T>
T * Array<T>::ToMutableContentPtr( const T * ptr )
{
    if ( ptr == nullptr )
    {
        return nullptr;
    }
    const size_t offset = GetIndexOf( ptr );
    EnsureIsOnlyOwner();
    return ( m_Begin + offset );
}

// Grow
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Grow()
{
    Grow( GetCapacity() + 1 );
}

// Grow (size_t)
//------------------------------------------------------------------------------
template <class T>
void Array<T>::Grow( size_t minimumSize )
{
    // grow by 1.5 times (but at least by one)
    const size_t oldCapacity = (size_t)m_Capacity;
    const size_t newCapacity = Math::Max( minimumSize, ( oldCapacity + ( oldCapacity >> 1 ) + 1 ) );
    ASSERT( newCapacity > oldCapacity ); // detect overflow
    SetCapacity( newCapacity );
}

// Allocate
//------------------------------------------------------------------------------
template <class T>
/*static*/ void Array<T>::Allocate( Atomic<uint32_t> * & referenceCount, T * & begin, size_t numElements )
{
    if ( numElements == 0 )
    {
        referenceCount = nullptr;
        begin = nullptr;
    }

    constexpr size_t align = Array<T>::s_Align;
    constexpr size_t beginOffset = Array<T>::s_BeginOffset;
    const size_t size = beginOffset + sizeof( T ) * numElements;
    void * const mem = ALLOC( size, align );
    referenceCount = reinterpret_cast<Atomic<uint32_t> *>( mem );
    begin = reinterpret_cast<T *>( (char *)mem + beginOffset );
    if constexpr ( Array<T>::s_TIsShareable )
    {
        INPLACE_NEW( referenceCount ) Atomic<uint32_t>( 1 );
    }
}

// Deallocate
//------------------------------------------------------------------------------
template <class T>
/*static*/ void Array<T>::Release( Atomic<uint32_t> * referenceCount, const T * begin, const T * end )
{
    ASSERT( begin <= end );
    if ( referenceCount )
    {
        // If we share the memory with other arrays, we must NOT be pointing to different places in memory
        ASSERT( (void *)begin == (void *)( (char *)referenceCount + Array<T>::s_BeginOffset ) );

        // Memory is on the heap, but may be shared with others
        const uint32_t remainingReferenceCount = Array<T>::s_TIsShareable ? referenceCount->Decrement() : 0;
        if ( remainingReferenceCount == 0 )
        {
            // We were the last remaining string owning this memory, so it must be freed
            if constexpr ( Array<T>::s_TIsShareable )
            {
                referenceCount->~Atomic<uint32_t>();
            }

            // Destroy all items
            Array<T>::Delete( begin, end );

            FREE( (void *)referenceCount );
        }
    }
    else
    {
        Array<T>::Delete( begin, end );
    }
}

// Delete
//------------------------------------------------------------------------------
template <class T>
/*static*/ void Array<T>::Delete( const T * begin, const T * end )
{
    ASSERT( begin <= end );
    for ( const T * it = begin; it < end; ++it )
    {
        it->~T();
    }
}

// EnsureIsOnlyOwner
//------------------------------------------------------------------------------
template <class T>
void Array<T>::EnsureIsOnlyOwner()
{
    EnsureIsOnlyOwner( GetSize() );
}

// EnsureIsOnlyOwner (size_t)
//------------------------------------------------------------------------------
template <class T>
void Array<T>::EnsureIsOnlyOwner( size_t capacity )
{
    if ( !IsOnlyOwner() )
    {
        // This array's data is shared with another array. Create a copy.
        CopyToSharedData( capacity );
    }
}

// CopyToSharedData
//------------------------------------------------------------------------------
template <class T>
void Array<T>::CopyToSharedData( size_t capacity )
{
    Atomic<uint32_t> * const oldReferenceCount = m_ReferenceCount;
    T * const oldBegin = m_Begin;
    T * const oldEnd = m_Begin + m_Size;

    // hook up to new memory
    Array<T>::Allocate( m_ReferenceCount, m_Begin, capacity );
    m_Capacity = capacity;
    T * const newBegin = m_Begin;
    T * const newEnd = m_Begin + m_Capacity;

    // initialize items in new location
    const bool doMove = ( oldReferenceCount == nullptr ) || ( oldReferenceCount->Load() == 1 );
    for ( T * src = oldBegin, * dst = newBegin; src < oldEnd && dst < newEnd; ++src, ++dst )
    {
        if constexpr ( !s_TIsShareable )
        {
            // this array is the last owner, so move the items
            INPLACE_NEW( dst ) T( Move( *src ) );
        }
        else
        {
            if ( doMove )
            {
                // this array is the last owner, so move the items
                INPLACE_NEW( dst ) T( Move( *src ) );
            }
            else
            {
                // this array shares data with other arrays, so copy the items
                INPLACE_NEW( dst ) T( *src );
            }
        }
    }

    // release old memory and maybe free and destroy the old items
    Array<T>::Release( oldReferenceCount, oldBegin, oldEnd );
}

// StackArray
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED = 32>
class StackArray : public Array<T>
{
    static_assert( RESERVED > 0, "StackArray must have a non-zero reserved size" );
public:
    StackArray();
    StackArray( const StackArray<T> & other );
    StackArray( StackArray<T> && other );
    StackArray( const Array<T> & other );
    StackArray( Array<T> && other );
    ~StackArray() = default;

    StackArray& operator=( const StackArray<T> & other );
    StackArray& operator=( StackArray<T> && other );
    StackArray& operator=( const Array<T> & other );
    StackArray& operator=( Array<T> && other );

private:
    PRAGMA_DISABLE_PUSH_MSVC( 4324 ) // structure was padded due to alignment specifier
    alignas( __alignof( T ) ) uint8_t m_Storage[ RESERVED * sizeof( T ) ];
    PRAGMA_DISABLE_POP_MSVC // 4324
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED>::StackArray()
{
    ASSERT( Array<T>::m_ReferenceCount == nullptr );
    ASSERT( Array<T>::m_Begin == nullptr );
    ASSERT( Array<T>::m_Size == 0 );
    ASSERT( Array<T>::m_Capacity == 0 );
    Array<T>::m_Begin = reinterpret_cast<T *>( &m_Storage );
    Array<T>::m_Capacity = RESERVED;
    ASSERT( !Array<T>::IsUsingSharedMemory() );
}

// CONSTRUCTOR (const StackArray &)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED>::StackArray( const Array<T> & other )
    : StackArray()
{
    Array<T>::operator=( other );
}

// CONSTRUCTOR (StackArray &&)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED>::StackArray( Array<T> && other )
    : StackArray()
{
    Array<T>::operator=( Move( other ) );
}

// CONSTRUCTOR (const Array &)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED>::StackArray( const StackArray<T> & other )
    : StackArray()
{
    Array<T>::operator=( other );
}

// CONSTRUCTOR (Array &&)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED>::StackArray( StackArray<T> && other )
    : StackArray()
{
    Array<T>::operator=( Move( other ) );
}

// operator = (const StackArray &)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED> & StackArray<T, RESERVED>::operator=( const StackArray<T> & other )
{
    Array<T>::operator=( other );
    return *this;
}

// operator = (StackArray &&)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED> & StackArray<T, RESERVED>::operator=( StackArray<T> && other )
{
    Array<T>::operator=( Move( other ) );
    return *this;
}

// operator = (const Array &)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED> & StackArray<T, RESERVED>::operator=( const Array<T> & other )
{
    Array<T>::operator=( other );
    return *this;
}

// operator = (Array &&)
//------------------------------------------------------------------------------
template <class T, uint32_t RESERVED>
StackArray<T, RESERVED> & StackArray<T, RESERVED>::operator=( Array<T> && other )
{
    Array<T>::operator=( Move( other ) );
    return *this;
}

//------------------------------------------------------------------------------
