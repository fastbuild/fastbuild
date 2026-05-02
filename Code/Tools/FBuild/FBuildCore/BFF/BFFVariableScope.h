// BFFVariableScope.h - the collection of variables within a single scope or struct
//------------------------------------------------------------------------------
#pragma once

// Forward Declarations
//------------------------------------------------------------------------------
class BFFVariable;

// Core
#include "Core/Containers/Array.h"
#include "Core/Containers/UnorderedMap.h"
#include "Core/Strings/AString.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/Mem.h"

// BFFVariableScope
//------------------------------------------------------------------------------
class BFFVariableScope
{
public:
    BFFVariableScope();
    BFFVariableScope( const BFFVariableScope & other );
    BFFVariableScope( BFFVariableScope && other );
    ~BFFVariableScope();

    BFFVariableScope & operator=( const BFFVariableScope & other );
    BFFVariableScope & operator=( BFFVariableScope && other );

    const BFFVariable * Find( const AString & name ) const;
    BFFVariable * Find( const AString & name );

    template <class... ARGS>
    BFFVariable & Emplace( const AString & name, ARGS &&... args );

    void Clear();

    enum class Mode
    {
        ARRAY,
        MAP,
    };

    template <class ARRAY_ITERATOR, class MAP_ITERATOR, class VARIABLE>
    class IteratorTemplate
    {
    public:
        IteratorTemplate( ARRAY_ITERATOR iterator )
            : m_Mode( Mode::ARRAY )
        {
            INPLACE_NEW( reinterpret_cast<ARRAY_ITERATOR *>( &m_Iterator ) ) ARRAY_ITERATOR( iterator );
        }
        IteratorTemplate( MAP_ITERATOR iterator )
            : m_Mode( Mode::MAP )
        {
            INPLACE_NEW( reinterpret_cast<MAP_ITERATOR *>( &m_Iterator ) ) MAP_ITERATOR( iterator );
        }

        ~IteratorTemplate() = default;

        bool operator==( const IteratorTemplate & other ) const
        {
            if ( m_Mode != other.m_Mode )
            {
                return false;
            }
            switch ( m_Mode )
            {
                case Mode::ARRAY: return ( *GetArrayIterator() ) == ( *other.GetArrayIterator() );
                case Mode::MAP: return ( *GetMapIterator() ) == ( *other.GetMapIterator() );
            }
        }

        bool operator!=( const IteratorTemplate & other ) const { return !( ( *this ) == other ); }

        IteratorTemplate & operator++()
        {
        if ( ARRAY_ITERATOR * iterator = GetArrayIterator() )
        {
            ++( *iterator );
        }
        else if ( MAP_ITERATOR * iterator = GetMapIterator() )
        {
            ++( *iterator );
        }
        return *this;
        }

        IteratorTemplate operator++(int) { IteratorTemplate tmp = *this; ++( *this ); return tmp; }
        IteratorTemplate & operator=( const IteratorTemplate & other )
        {
            if ( &other == this )
            {
                return *this;
            }
            if ( m_Mode != other.m_Mode )
            {
                Destruct();
            }
            m_Mode = other.m_Mode;
            switch ( m_Mode )
            {
                case Mode::ARRAY: INPLACE_NEW( GetArrayIterator() ) ARRAY_ITERATOR( other );
                case Mode::MAP: INPLACE_NEW( GetMapIterator() ) MAP_ITERATOR( other );
            }
            return *this;
        }

        VARIABLE & operator*() const
        {
            switch ( m_Mode )
            {
                case Mode::ARRAY: return **GetArrayIterator();
                case Mode::MAP: return ( **GetMapIterator() ).m_Value;
            }
        }

    private:
        void Destruct()
        {
            if ( ARRAY_ITERATOR * iterator = GetArrayIterator() )
            {
                iterator->~ARRAY_ITERATOR();
            }
            else if ( MAP_ITERATOR * iterator = GetMapIterator() )
            {
                iterator->~MAP_ITERATOR();
            }
        }

        ARRAY_ITERATOR * GetArrayIterator() const
        {
            if ( m_Mode == Mode::ARRAY )
            {
                return reinterpret_cast<ARRAY_ITERATOR *>( const_cast<void *>( (const void *)&m_Iterator ) );
            }
            return nullptr;
        }

        MAP_ITERATOR * GetMapIterator() const
        {
            if ( m_Mode == Mode::MAP )
            {
                return reinterpret_cast<MAP_ITERATOR *>( const_cast<void *>( (const void *)&m_Iterator ) );
            }
            return nullptr;
        }

        Mode m_Mode;
        alignas( Math::Max( __alignof( ARRAY_ITERATOR ), __alignof( MAP_ITERATOR ) ) )
        uint8_t m_Iterator[ Math::Max( sizeof( ARRAY_ITERATOR ), sizeof( MAP_ITERATOR ) ) ];
    };

    static constexpr uint32_t maxArraySize = 32;

    using ArrayType = Array<BFFVariable>;
    using MapType = UnorderedMap<AString, BFFVariable>;

    using Iterator = IteratorTemplate<ArrayType::Iter, MapType::Iterator, BFFVariable>;
    using ConstIterator = IteratorTemplate<ArrayType::ConstIter, MapType::ConstIterator, const BFFVariable>;

    Iterator begin()
    {
        switch ( m_Mode )
        {
            case Mode::ARRAY: return Iterator( GetArray()->begin() );
            case Mode::MAP: return Iterator( GetMap()->begin() );
        }
    }

    Iterator end()
    {
        switch ( m_Mode )
        {
            case Mode::ARRAY: return Iterator( GetArray()->end() );
            case Mode::MAP: return Iterator( GetMap()->end() );
        }
    }

    ConstIterator begin() const
    {
        switch ( m_Mode )
        {
            case Mode::ARRAY: return ConstIterator( GetArray()->begin() );
            case Mode::MAP: return ConstIterator( GetMap()->begin() );
        }
    }

    ConstIterator end() const
    {
        switch ( m_Mode )
        {
            case Mode::ARRAY: return ConstIterator( GetArray()->end() );
            case Mode::MAP: return ConstIterator( GetMap()->end() );
        }
    }

private:
    void Destruct();
    void MaybeSwitchToMap();

    const ArrayType * GetArray() const;
    ArrayType * GetArray();
    const MapType * GetMap() const;
    MapType * GetMap();

    Mode m_Mode;
    alignas( Math::Max( __alignof( ArrayType ), __alignof( MapType ) ) )
    uint8_t m_Variables[ Math::Max( sizeof( ArrayType ), sizeof( MapType ) ) ];
};

// EMPLACE
//------------------------------------------------------------------------------
template <class... ARGS>
BFFVariable & BFFVariableScope::Emplace( const AString & name, ARGS &&... args )
{
    MaybeSwitchToMap();
    if ( ArrayType * array = GetArray() )
    {
        return array->EmplaceBack( Forward( ARGS, args )... );
    }
    else
    {
        MapType * map = GetMap();
        ASSERT( map );
        return map->Emplace( name, Forward( ARGS, args )... ).m_Value;
    }
}

//------------------------------------------------------------------------------
