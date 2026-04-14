// UnorderedMap.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Array.h"
#include "Core/Env/Types.h"
#include "Core/Math/xxHash.h"

// Hashing Functions
//------------------------------------------------------------------------------
namespace UnorderedMapKeyHashingFunctions
{
    inline uint32_t Hash( const AString & key )
    {
        return xxHash3::Calc32( key );
    }
}

// UnorderedMap
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
class UnorderedMap
{
public:
    UnorderedMap();
    UnorderedMap( const UnorderedMap<KEY, VALUE> & other );
    UnorderedMap( UnorderedMap<KEY, VALUE> && other );
    ~UnorderedMap() = default;

    void Destruct();
    void Clear() { Destruct(); }

    [[nodiscard]] bool IsEmpty() const { return ( m_Count == 0 ); }
    [[nodiscard]] size_t GetSize() const { return m_Count; }

    UnorderedMap<KEY, VALUE> & operator=( const UnorderedMap<KEY, VALUE> & other );
    UnorderedMap<KEY, VALUE> & operator=( UnorderedMap<KEY, VALUE> && other );

    class KeyValue
    {
    public:
        template <class... ARGS>
        KeyValue( const KEY & key, ARGS &&... args )
            : m_Key( key )
            , m_Value( Forward( ARGS, args )... )
        {
        }

        KeyValue( const KeyValue & other )
            : m_Key( other.m_Key )
            , m_Value( other.m_Value )
        {
        }

        KeyValue( KeyValue && other )
            : m_Key( Move( other.m_Key ) )
            , m_Value( Move( other.m_Value ) )
        {
        }

        KeyValue & operator=( const KeyValue & other ) = delete;

        bool operator==( const KEY & other ) const { return ( m_Key == other ); }

        const KEY m_Key;
        VALUE m_Value;

    protected:
        friend class UnorderedMap<KEY, VALUE>;
    };

    // Check if an item exists in the map
    [[nodiscard]] KeyValue * Find( const KEY & key );
    [[nodiscard]] const KeyValue * Find( const KEY & key ) const;

    // Add items to the map
    KeyValue & Insert( const KEY & key, const VALUE & value );
    template <class... ARGS>
    KeyValue & Emplace( const KEY & key, ARGS &&... args );

    template <class ARRAY>
    class IteratorTemplate
    {
    public:
        using BaseValueType = typename ARRAY::ValueType::ValueType;
        using ValueType = std::conditional_t<std::is_const_v<ARRAY>, std::add_const_t<BaseValueType>, BaseValueType>;
        using OuterIterator = std::conditional_t<std::is_const_v<ARRAY>, typename ARRAY::ConstIter, typename ARRAY::Iter>;
        using InnerIterator = std::conditional_t<std::is_const_v<ARRAY>, typename ARRAY::ValueType::ConstIter, typename ARRAY::ValueType::Iter>;

    private:
        IteratorTemplate( OuterIterator outerIter, OuterIterator outerIterEnd )
            : m_OuterIter( outerIter )
            , m_OuterIterEnd( outerIterEnd )
            , m_InnerIter( outerIter != outerIterEnd ? outerIter->Begin() : InnerIterator() )
            , m_InnerIterEnd( outerIter != outerIterEnd ? outerIter->End() : InnerIterator() )
        {
        }

    public:
        IteratorTemplate( const IteratorTemplate & other ) = default;
        IteratorTemplate( IteratorTemplate && other ) = default;
        IteratorTemplate& operator=( const IteratorTemplate & other ) = default;
        IteratorTemplate& operator=( IteratorTemplate && other ) = default;

        ValueType & operator*() const;
        IteratorTemplate operator++( int );
        IteratorTemplate& operator++();

        bool operator==( const IteratorTemplate & other ) const;
        bool operator!=( const IteratorTemplate & other ) const { return !( ( *this ) == other ); }

        static IteratorTemplate<ARRAY> BeginningOf( ARRAY & buckets );
        static IteratorTemplate<ARRAY> EndOf( ARRAY & buckets );

    private:
        OuterIterator m_OuterIter;
        OuterIterator m_OuterIterEnd;
        InnerIterator m_InnerIter;
        InnerIterator m_InnerIterEnd;
    };

    using Iterator = IteratorTemplate<Array<Array<KeyValue>>>;
    using ConstIterator = IteratorTemplate<const Array<Array<KeyValue>>>;

    Iterator Begin() { return Iterator::BeginningOf( m_Buckets ); }
    Iterator End() { return Iterator::EndOf( m_Buckets ); }
    ConstIterator Begin() const { return ConstIterator::BeginningOf( m_Buckets ); }
    ConstIterator End() const { return ConstIterator::EndOf( m_Buckets ); }

    Iterator begin() { return Iterator::BeginningOf( m_Buckets ); }
    Iterator end() { return Iterator::EndOf( m_Buckets ); }
    ConstIterator begin() const { return ConstIterator::BeginningOf( m_Buckets ); }
    ConstIterator end() const { return ConstIterator::EndOf( m_Buckets ); }

protected:
    void EnsureBucketsAllocated();

    inline static constexpr uint32_t kTableSizePower = 8;
    inline static constexpr uint32_t kTableSize = ( 1 << kTableSizePower );
    inline static constexpr uint32_t kTableSizeMask = ( kTableSize - 1 );

    Array<Array<KeyValue>> m_Buckets;
    uint32_t m_Count = 0;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE>::UnorderedMap()
  : m_Buckets()
  , m_Count( 0 )
{
}

// CONSTRUCTOR (copy)
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE>::UnorderedMap( const UnorderedMap & other )
    : m_Buckets( other.m_Buckets )
    , m_Count( other.m_Count )
{
}

// CONSTRUCTOR (move)
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE>::UnorderedMap( UnorderedMap && other )
    : m_Buckets( Move( other.m_Buckets ) )
    , m_Count( Move( other.m_Count ) )
{
    other.Destruct();
}

// DESTRUCT
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
void UnorderedMap<KEY, VALUE>::Destruct()
{
    m_Buckets.Destruct();
    m_Count = 0;
}

// Copy assign
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE> & UnorderedMap<KEY, VALUE>::operator=( const UnorderedMap<KEY, VALUE> & other )
{
    // self-assignment should be a no-op.
    if ( &other == this )
    {
        return *this;
    }

    m_Buckets = other.m_Buckets;
    m_Count = other.m_Count;
    return *this;
}

// Move assign
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE> & UnorderedMap<KEY, VALUE>::operator=( UnorderedMap<KEY, VALUE> && other )
{
    // self-assignment should be a no-op.
    if ( &other == this )
    {
        return *this;
    }

    m_Buckets = Move( other.m_Buckets );
    m_Count = Move( other.m_Count );
    other.Destruct();

    return *this;
}

// Find
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
typename UnorderedMap<KEY, VALUE>::KeyValue * UnorderedMap<KEY, VALUE>::Find( const KEY & key )
{
    // Handle empty
    if ( m_Buckets.IsEmpty() )
    {
        return nullptr;
    }

    // Hash the key
    const uint32_t hash = UnorderedMapKeyHashingFunctions::Hash( key );

    // Find the bucket
    const uint32_t bucketId = ( hash & kTableSizeMask );
    // Search using const so we avoid creating unnecessary eager copies
    const Array<KeyValue> & bucket = const_cast<const UnorderedMap *>( this )->m_Buckets[ bucketId ];
    const KeyValue * keyValue = bucket.Find( key );
    if ( keyValue )
    {
        const size_t index = const_cast<const UnorderedMap *>( this )->m_Buckets[ bucketId ].GetIndexOf( keyValue );
        return &m_Buckets[ bucketId ][ index ];
    }
    return nullptr;
}

// Find
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
const typename UnorderedMap<KEY, VALUE>::KeyValue * UnorderedMap<KEY, VALUE>::Find( const KEY & key ) const
{
    // Handle empty
    if ( m_Buckets.IsEmpty() )
    {
        return nullptr;
    }

    // Hash the key
    const uint32_t hash = UnorderedMapKeyHashingFunctions::Hash( key );

    // Find the bucket
    const uint32_t bucketId = ( hash & kTableSizeMask );
    const Array<KeyValue> & bucket = m_Buckets[ bucketId ];
    return bucket.Find( key );
}

// Insert
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
typename UnorderedMap<KEY, VALUE>::KeyValue & UnorderedMap<KEY, VALUE>::Insert( const KEY & key, const VALUE & value )
{
    return Emplace( key, value );
}

// Emplace
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class... ARGS>
typename UnorderedMap<KEY, VALUE>::KeyValue & UnorderedMap<KEY, VALUE>::Emplace( const KEY & key, ARGS &&... args )
{
    // Handle empty
    EnsureBucketsAllocated();

    // Hash the key
    const uint32_t hash = UnorderedMapKeyHashingFunctions::Hash( key );

    // Find the bucket
    const uint32_t bucketId = ( hash & kTableSizeMask );
    Array<KeyValue> & bucket = m_Buckets[ bucketId ];

    // Debug check item doesn't already exist
#if defined( ASSERTS_ENABLED )
    for ( const KeyValue & existingKeyValue : bucket )
    {
        ASSERT( existingKeyValue.m_Key != key );
    }
#endif

    // Create storage for new item
    m_Count++;
    return bucket.EmplaceBack( key, Forward( ARGS, args )... );
}

// EnsureBucketsAllocated
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
void UnorderedMap<KEY, VALUE>::EnsureBucketsAllocated()
{
    m_Buckets.SetSize( kTableSize );
}

// Iterator operator *
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class ARRAY>
typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<ARRAY>::ValueType & UnorderedMap<KEY, VALUE>::IteratorTemplate<ARRAY>::operator*() const
{
    return *m_InnerIter;
}

// Iterator postfix increment
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class ARRAY>
typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<ARRAY> UnorderedMap<KEY, VALUE>::IteratorTemplate<ARRAY>::operator++( int )
{
    IteratorTemplate<ARRAY> copy( *this );
    ++( *this );
    return copy;
}

// Iterator prefix increment
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class ARRAY>
typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<ARRAY> & UnorderedMap<KEY, VALUE>::IteratorTemplate<ARRAY>::operator++()
{
    if ( m_InnerIter == m_InnerIterEnd )
    {
        // No where safe to go, so do nothing and return
        return *this;
    }

    ++m_InnerIter;
    while ( ( m_InnerIter == m_InnerIterEnd ) && ( m_OuterIter != m_OuterIterEnd ) )
    {
        ++m_OuterIter;
        if ( m_OuterIter != m_OuterIterEnd )
        {
            m_InnerIter = ( *m_OuterIter ).Begin();
            m_InnerIterEnd = ( *m_OuterIter ).End();
        }
        else
        {
            m_InnerIter = InnerIterator();
            m_InnerIterEnd = InnerIterator();
        }
    }

    ASSERT( ( m_InnerIter != m_InnerIterEnd ) || ( m_OuterIter == m_OuterIterEnd ) );
    return *this;
}

// Iterator operator *
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class ARRAY>
bool UnorderedMap<KEY, VALUE>::IteratorTemplate<ARRAY>::operator==( const IteratorTemplate<ARRAY> & other ) const
{
    return ( ( m_OuterIter == other.m_OuterIter ) && ( m_InnerIter == other.m_InnerIter ) );
}

// Iterator begin
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class ARRAY>
/*static*/ typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<ARRAY> UnorderedMap<KEY, VALUE>::IteratorTemplate<ARRAY>::BeginningOf( ARRAY & buckets )
{
    if ( buckets.IsEmpty() )
    {
        return IteratorTemplate<ARRAY>( buckets.Begin(), buckets.End() );
    }

    const OuterIterator end = buckets.End();
    OuterIterator iter = buckets.Begin();
    while ( ( iter != end ) && ( *iter ).IsEmpty() )
    {
        ++iter;
    }

    return IteratorTemplate<ARRAY>( iter, end );
}

// Iterator end
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class ARRAY>
/*static*/ typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<ARRAY> UnorderedMap<KEY, VALUE>::IteratorTemplate<ARRAY>::EndOf( ARRAY & buckets )
{
    return IteratorTemplate<ARRAY>( buckets.End(), buckets.End() );
}

//------------------------------------------------------------------------------
