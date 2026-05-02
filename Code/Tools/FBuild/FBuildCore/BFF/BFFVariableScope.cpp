// BFFVariableScope
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "BFFVariableScope.h"
#include "BFFVariable.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFVariableScope::BFFVariableScope()
{
    m_Mode = Mode::ARRAY;
    INPLACE_NEW( reinterpret_cast<ArrayType *>( &m_Variables ) ) ArrayType();
}

// CONSTRUCTOR (copy)
//------------------------------------------------------------------------------
BFFVariableScope::BFFVariableScope( const BFFVariableScope & other )
{
    if ( const ArrayType * array = other.GetArray() )
    {
        INPLACE_NEW( reinterpret_cast<ArrayType *>( &m_Variables ) ) ArrayType( *array );
    }
    else if ( const MapType * map = other.GetMap() )
    {
        INPLACE_NEW( reinterpret_cast<MapType *>( &m_Variables ) ) MapType( *map );
    }
    m_Mode = other.m_Mode;
}

// CONSTRUCTOR (move)
//------------------------------------------------------------------------------
BFFVariableScope::BFFVariableScope( BFFVariableScope && other )
{
    if ( ArrayType * array = other.GetArray() )
    {
        INPLACE_NEW( reinterpret_cast<ArrayType *>( &m_Variables ) ) ArrayType( Move( *array ) );
    }
    else if ( MapType * map = other.GetMap() )
    {
        INPLACE_NEW( reinterpret_cast<MapType *>( &m_Variables ) ) MapType( Move( *map ) );
    }
    m_Mode = other.m_Mode;
}

// CONSTRUCTOR (move)
//------------------------------------------------------------------------------
BFFVariableScope::~BFFVariableScope()
{
    Destruct();
}

// ASSIGNMENT (copy)
//------------------------------------------------------------------------------
BFFVariableScope & BFFVariableScope::operator=( const BFFVariableScope & other )
{
    // Self-assignment is a no-op
    if ( &other == this )
    {
        return *this;
    }
    if ( m_Mode != other.m_Mode )
    {
        Destruct();
        if ( const ArrayType * array = other.GetArray() )
        {
            INPLACE_NEW( reinterpret_cast<ArrayType *>( &m_Variables ) ) ArrayType( *array );
        }
        else if ( const MapType * map = other.GetMap() )
        {
            INPLACE_NEW( reinterpret_cast<MapType *>( &m_Variables ) ) MapType( *map );
        }
        m_Mode = other.m_Mode;
    }
    else
    {
        if ( ArrayType * array = GetArray() )
        {
            ( *array ) = *other.GetArray();
        }
        else if ( MapType * map = GetMap() )
        {
            ( *map ) = *other.GetMap();
        }
    }
    return *this;
}

// ASSIGNMENT (move)
//------------------------------------------------------------------------------
BFFVariableScope & BFFVariableScope::operator=( BFFVariableScope && other )
{
    // Self-assignment is a no-op
    if ( &other == this )
    {
        return *this;
    }
    if ( m_Mode != other.m_Mode )
    {
        Destruct();
        if ( ArrayType * array = other.GetArray() )
        {
            INPLACE_NEW( reinterpret_cast<ArrayType *>( &m_Variables ) ) ArrayType( Move( *array ) );
        }
        else if ( MapType * map = other.GetMap() )
        {
            INPLACE_NEW( reinterpret_cast<MapType *>( &m_Variables ) ) MapType( Move( *map ) );
        }
        m_Mode = other.m_Mode;
    }
    else
    {
        if ( ArrayType * array = GetArray() )
        {
            ( *array ) = Move( *other.GetArray() );
        }
        else if ( MapType * map = GetMap() )
        {
            ( *map ) = Move( *other.GetMap() );
        }
    }
    return *this;
}

// FIND
//------------------------------------------------------------------------------
const BFFVariable * BFFVariableScope::Find( const AString & name ) const
{
    return const_cast<const BFFVariable *>( const_cast<BFFVariableScope *>( this )->Find( name ) );
}

// FIND
//------------------------------------------------------------------------------
BFFVariable * BFFVariableScope::Find( const AString & name )
{
    if ( ArrayType * array = GetArray() )
    {
        for ( BFFVariable & var : *array )
        {
            if ( var.GetName() == name )
            {
                return &var;
            }
        }
    }
    else if ( MapType * map = GetMap() )
    {
        if ( UnorderedMap<AString, BFFVariable>::KeyValue * keyValue = map->Find( name ) )
        {
            return &keyValue->m_Value;
        }
    }
    return nullptr;
}

// CLEAR
//------------------------------------------------------------------------------
void BFFVariableScope::Clear()
{
    if ( ArrayType * array = GetArray() )
    {
        array->Clear();
    }
    else if ( MapType * map = GetMap() )
    {
        map->Clear();
    }
}

// DESTRUCT
//------------------------------------------------------------------------------
void BFFVariableScope::Destruct()
{
    switch ( m_Mode )
    {
        case BFFVariableScope::Mode::ARRAY:
            reinterpret_cast<ArrayType *>( &m_Variables )->~ArrayType();
            break;
        case BFFVariableScope::Mode::MAP:
            reinterpret_cast<MapType *>( &m_Variables )->~MapType();
            break;
    }
}

// MaybeSwitchToMap
//------------------------------------------------------------------------------
void BFFVariableScope::MaybeSwitchToMap()
{
    ArrayType * array = GetArray();
    if ( !array )
    {
        return;
    }
    if ( array->GetSize() < maxArraySize )
    {
        return;
    }

    MapType newMap;
    for ( BFFVariable & var : *array )
    {
        newMap.Emplace( var.GetName(), Move( var ) );
    }

    Destruct();
    INPLACE_NEW( reinterpret_cast<MapType *>( &m_Variables ) ) MapType( Move( newMap ) );
    m_Mode = Mode::MAP;
}

// ARRAY
//------------------------------------------------------------------------------
const BFFVariableScope::ArrayType * BFFVariableScope::GetArray() const
{
    if ( m_Mode == Mode::ARRAY )
    {
        return reinterpret_cast<const ArrayType *>( &m_Variables );
    }
    return nullptr;
}

// ARRAY
//------------------------------------------------------------------------------
BFFVariableScope::ArrayType * BFFVariableScope::GetArray()
{
    if ( m_Mode == Mode::ARRAY )
    {
        return reinterpret_cast<ArrayType *>( &m_Variables );
    }
    return nullptr;
}

// MAP
//------------------------------------------------------------------------------
const BFFVariableScope::MapType * BFFVariableScope::GetMap() const
{
    if ( m_Mode == Mode::MAP )
    {
        return reinterpret_cast<const MapType *>( &m_Variables );
    }
    return nullptr;
}

// MAP
//------------------------------------------------------------------------------
BFFVariableScope::MapType * BFFVariableScope::GetMap()
{
    if ( m_Mode == Mode::MAP )
    {
        return reinterpret_cast<MapType *>( &m_Variables );
    }
    return nullptr;
}

//------------------------------------------------------------------------------
