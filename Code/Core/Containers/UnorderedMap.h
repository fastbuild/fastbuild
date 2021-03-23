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
        return xxHash::Calc32( key );
    }
}

// UnorderedMap
//------------------------------------------------------------------------------
template< class KEY, class VALUE >
class UnorderedMap
{
public:
    UnorderedMap();
    UnorderedMap( UnorderedMap< KEY, VALUE > && other ) = delete;
    ~UnorderedMap();

    void Destruct();

    bool    IsEmpty() const { return ( m_Count == 0 ); }
    size_t  GetSize() const { return m_Count; }

    UnorderedMap< KEY, VALUE > & operator = ( const UnorderedMap< KEY, VALUE > & other ) = delete;
    UnorderedMap< KEY, VALUE > & operator = ( UnorderedMap< KEY, VALUE > && other ) = delete;

    class KeyValue
    {
    public:
        KeyValue( const KEY & key, const VALUE & value, KeyValue * next )
            : m_Key( key )
            , m_Value( value )
            , m_Next( next )
        {}

        KeyValue & operator = ( const KeyValue & other ) = delete;

        const KEY   m_Key;
        VALUE       m_Value;

    protected:
        friend class UnorderedMap< KEY, VALUE >;
        KeyValue    * m_Next; // In-place linked list for each bucket
    };

    // Check if an item exists in the map
    [[nodiscard]] KeyValue *    Find( const KEY & key );

    // Add items to the map
    KeyValue &    Insert( const KEY & key, const VALUE & value );

protected:
    enum : uint32_t { kTableSizePower = 16 };
    enum : uint32_t { kTableSize = ( 1 << kTableSizePower ) };
    enum : uint32_t { kTableSizeMask = ( kTableSize - 1 ) };

    KeyValue ** m_Buckets = nullptr;
    uint32_t m_Count = 0;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template< class KEY, class VALUE >
UnorderedMap< KEY, VALUE >::UnorderedMap() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
template< class KEY, class VALUE >
UnorderedMap< KEY, VALUE >::~UnorderedMap()
{
    Destruct();
}

// Destruct
//------------------------------------------------------------------------------
template< class KEY, class VALUE >
void UnorderedMap< KEY, VALUE >::Destruct()
{
    if ( m_Buckets )
    {
        for ( uint32_t i = 0 ; i < kTableSize ; ++i )
        {
            KeyValue * existingKeyValue = m_Buckets[ i ];
            while ( existingKeyValue )
            {
                KeyValue * next = existingKeyValue->m_Next;
                FDELETE existingKeyValue;
                existingKeyValue = next;
            }
        }

        FDELETE [] m_Buckets;
        m_Buckets = nullptr;
    }
    m_Count = 0;
}

// Find
//------------------------------------------------------------------------------
template< class KEY, class VALUE >
typename UnorderedMap< KEY, VALUE >::KeyValue * UnorderedMap< KEY, VALUE >::Find( const KEY & key )
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
    KeyValue * keyValue = m_Buckets[ bucketId ];

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
template< class KEY, class VALUE >
typename UnorderedMap< KEY, VALUE >::KeyValue & UnorderedMap< KEY, VALUE >::Insert( const KEY & key, const VALUE & value )
{
    // Handle empty
    if ( m_Buckets == nullptr )
    {
        m_Buckets = FNEW( KeyValue *[ kTableSize ]() ); // NOTE: zero initialized
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

//------------------------------------------------------------------------------
