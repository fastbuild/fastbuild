// UnorderedMap.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
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
    UnorderedMap( UnorderedMap<KEY, VALUE> && other );
    ~UnorderedMap();

    void Destruct();

    [[nodiscard]] bool IsEmpty() const { return ( m_Count == 0 ); }
    [[nodiscard]] size_t GetSize() const { return m_Count; }

    UnorderedMap<KEY, VALUE> & operator=( const UnorderedMap<KEY, VALUE> & other ) = delete;
    UnorderedMap<KEY, VALUE> & operator=( UnorderedMap<KEY, VALUE> && other ) = delete;

    class KeyValue
    {
    public:
        KeyValue( const KEY & key, const VALUE & value, KeyValue * next )
            : m_Key( key )
            , m_Value( value )
            , m_Next( next )
        {
        }

        KeyValue & operator=( const KeyValue & other ) = delete;

        const KEY m_Key;
        VALUE m_Value;

    protected:
        friend class UnorderedMap<KEY, VALUE>;
        KeyValue * m_Next; // In-place linked list for each bucket
    };

    // Check if an item exists in the map
    [[nodiscard]] KeyValue * Find( const KEY & key );
    [[nodiscard]] const KeyValue * Find( const KEY & key ) const;

    // Add items to the map
    KeyValue & Insert( const KEY & key, const VALUE & value );

    template <class KEYVALUE>
    class IteratorTemplate
    {
    private:
        IteratorTemplate( KEYVALUE * const * buckets, uint32_t bucketIndex, KEYVALUE * keyValue )
            : m_Buckets( buckets )
            , m_BucketIndex( bucketIndex )
            , m_KeyValue( keyValue )
        {
        }

    public:
        IteratorTemplate( const IteratorTemplate & other ) = default;
        IteratorTemplate( IteratorTemplate && other ) = default;
        IteratorTemplate& operator=( const IteratorTemplate & other ) = default;
        IteratorTemplate& operator=( IteratorTemplate && other ) = default;

        KEYVALUE & operator*() const;
        IteratorTemplate operator++( int );
        IteratorTemplate& operator++();

        bool operator==( const IteratorTemplate & other ) const;

    private:
        static IteratorTemplate<KEYVALUE> BeginningOf( KEYVALUE * const * buckets );
        static IteratorTemplate<KEYVALUE> EndOf( KEYVALUE * const * buckets );

        KEYVALUE * const * m_Buckets;
        uint32_t m_BucketIndex;
        KEYVALUE * m_KeyValue;
    };

    using Iterator = IteratorTemplate<KeyValue>;
    using ConstIterator = IteratorTemplate<const KeyValue>;

    Iterator Begin() { return Iterator::BeginningOf( m_Buckets ); }
    Iterator End() { return Iterator::EndOf( m_Buckets ); }
    ConstIterator Begin() const { return ConstIterator::BeginningOf( m_Buckets ); }
    ConstIterator End() const { return ConstIterator::EndOf( m_Buckets ); }

protected:
    inline static constexpr uint32_t kTableSizePower = 16;
    inline static constexpr uint32_t kTableSize = ( 1 << kTableSizePower );
    inline static constexpr uint32_t kTableSizeMask = ( kTableSize - 1 );

    KeyValue ** m_Buckets = nullptr;
    uint32_t m_Count = 0;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE>::UnorderedMap() = default;

// CONSTRUCTOR (move)
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE>::UnorderedMap( UnorderedMap && other )
    : m_Buckets( other.m_Buckets )
    , m_Count( other.m_Count )
{
    other.m_Buckets = nullptr;
    other.m_Count = 0;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
UnorderedMap<KEY, VALUE>::~UnorderedMap()
{
    Destruct();
}

// Destruct
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
void UnorderedMap<KEY, VALUE>::Destruct()
{
    if ( m_Buckets )
    {
        for ( uint32_t i = 0; i < kTableSize; ++i )
        {
            KeyValue * existingKeyValue = m_Buckets[ i ];
            while ( existingKeyValue )
            {
                KeyValue * next = existingKeyValue->m_Next;
                FDELETE existingKeyValue;
                existingKeyValue = next;
            }
        }

        FDELETE[] m_Buckets;
        m_Buckets = nullptr;
    }
    m_Count = 0;
}

// Find
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
typename UnorderedMap<KEY, VALUE>::KeyValue * UnorderedMap<KEY, VALUE>::Find( const KEY & key )
{
    return const_cast<KeyValue *>( const_cast<const UnorderedMap<KEY, VALUE> *>( this )->Find( key ) );
}

// Find
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
const typename UnorderedMap<KEY, VALUE>::KeyValue * UnorderedMap<KEY, VALUE>::Find( const KEY & key ) const
{
    // Handle empty
    if ( m_Buckets == nullptr )
    {
        return nullptr;
    }

    // Hash the key
    const uint32_t hash = UnorderedMapKeyHashingFunctions::Hash( key );

    // Find the bucket
    const uint32_t bucketId = ( hash & kTableSizeMask );
    const KeyValue * keyValue = m_Buckets[ bucketId ];

    // Check entries in the bucket for exact key match
    while ( keyValue )
    {
        if ( keyValue->m_Key == key )
        {
            // Found
            return keyValue;
        }
        keyValue = keyValue->m_Next;
    }

    // Not found
    return nullptr;
}

// Insert
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
typename UnorderedMap<KEY, VALUE>::KeyValue & UnorderedMap<KEY, VALUE>::Insert( const KEY & key, const VALUE & value )
{
    // Handle empty
    if ( m_Buckets == nullptr )
    {
        m_Buckets = FNEW( KeyValue * [kTableSize]() ); // NOTE: zero initialized
    }

    // Hash the key
    const uint32_t hash = UnorderedMapKeyHashingFunctions::Hash( key );

    // Find the bucket
    const uint32_t bucketId = ( hash & kTableSizeMask );
    KeyValue * keyValue = m_Buckets[ bucketId ];

    // Debug check item doesn't already exist
#if defined( ASSERTS_ENABLED )
    KeyValue * existingKeyValue = keyValue;
    while ( existingKeyValue )
    {
        ASSERT( existingKeyValue->m_Key != key );
        existingKeyValue = existingKeyValue->m_Next;
    }
#endif

    // Create storage for new item
    KeyValue * newKeyValue = FNEW( KeyValue( key, value, keyValue ) );

    // Link into head of bucket
    m_Buckets[ bucketId ] = newKeyValue;

    m_Count++;

    // Return new item
    return *newKeyValue;
}

// Iterator operator *
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class KEYVALUE>
KEYVALUE & UnorderedMap<KEY, VALUE>::IteratorTemplate<KEYVALUE>::operator*() const
{
    ASSERT( m_KeyValue != nullptr );
    return *m_KeyValue;
}

// Iterator postfix increment
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class KEYVALUE>
typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<KEYVALUE> UnorderedMap<KEY, VALUE>::IteratorTemplate<KEYVALUE>::operator++( int )
{
    IteratorTemplate<KEYVALUE> copy( *this );
    ++( *this );
    return copy;
}

// Iterator prefix increment
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class KEYVALUE>
typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<KEYVALUE> & UnorderedMap<KEY, VALUE>::IteratorTemplate<KEYVALUE>::operator++()
{
    if ( m_Buckets == nullptr )
    {
        return *this;
    }
    if ( m_KeyValue == nullptr )
    {
        // No where safe to go, so do nothing and return
        ASSERT( m_BucketIndex == kTableSize );
        return *this;
    }

    m_KeyValue = m_KeyValue->m_Next;
    while ( ( m_KeyValue == nullptr ) && ( m_BucketIndex < kTableSize ) )
    {
        ++m_BucketIndex;
        m_KeyValue = m_BucketIndex < kTableSize ? m_Buckets[ m_BucketIndex ] : nullptr;
    }

    ASSERT( ( m_KeyValue != nullptr ) || ( m_BucketIndex == kTableSize ) );
    return *this;
}

// Iterator operator *
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class KEYVALUE>
bool UnorderedMap<KEY, VALUE>::IteratorTemplate<KEYVALUE>::operator==( const IteratorTemplate<KEYVALUE> & other ) const
{
    return ( ( m_Buckets == other.m_Buckets ) &&
             ( m_BucketIndex == other.m_BucketIndex ) &&
             ( m_KeyValue == other.m_KeyValue ) );
}

// Iterator begin
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class KEYVALUE>
/*static*/ typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<KEYVALUE> UnorderedMap<KEY, VALUE>::IteratorTemplate<KEYVALUE>::BeginningOf( KEYVALUE * const * buckets )
{
    if ( buckets == nullptr )
    {
        return IteratorTemplate<KEYVALUE>( nullptr, 0, nullptr );
    }
    for ( uint32_t i = 0; i < kTableSize; ++i )
    {
        if ( ( buckets[ i ] ) != nullptr )
        {
            return IteratorTemplate<KEYVALUE>( buckets, i, buckets[ i ] );
        }
    }
    return IteratorTemplate<KEYVALUE>::EndOf( buckets );
}

// Iterator end
//------------------------------------------------------------------------------
template <class KEY, class VALUE>
template <class KEYVALUE>
/*static*/ typename UnorderedMap<KEY, VALUE>::template IteratorTemplate<KEYVALUE> UnorderedMap<KEY, VALUE>::IteratorTemplate<KEYVALUE>::EndOf( KEYVALUE * const * buckets )
{
    if ( buckets == nullptr )
    {
        return IteratorTemplate<KEYVALUE>( nullptr, 0, nullptr );
    }
    return IteratorTemplate<KEYVALUE>( buckets, kTableSize, nullptr );
}

//------------------------------------------------------------------------------
