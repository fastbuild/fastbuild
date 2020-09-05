// ReflectionInfo.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ReflectionInfo.h"

// Core
#include "Core/Math/xxHash.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// System
#include <memory.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
ReflectionInfo::ReflectionInfo()
    : m_TypeNameCRC( 0 )
    , m_Properties( 0, true )
    , m_SuperClass( nullptr )
    , m_Next( nullptr )
    , m_TypeName( nullptr )
    , m_IsAbstract( false )
    , m_StructSize( 0 )
    , m_MetaDataChain( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ReflectionInfo::~ReflectionInfo()
{
    for ( ReflectedProperty * property : m_Properties )
    {
        delete property;
    }

    const IMetaData * md = m_MetaDataChain;
    while ( md )
    {
        const IMetaData * next = md->GetNext();
        FDELETE( md );
        md = next;
    }
}

// Begin
//------------------------------------------------------------------------------
ReflectionIter ReflectionInfo::Begin() const
{
    return ReflectionIter( this, 0 );
}

// End
//------------------------------------------------------------------------------
ReflectionIter ReflectionInfo::End() const
{
    return ReflectionIter( this, (uint32_t)m_Properties.GetSize() );
}

// GetReflectedProperty
//------------------------------------------------------------------------------
const ReflectedProperty & ReflectionInfo::GetReflectedProperty( uint32_t index ) const
{
    return *m_Properties[ index ];
}

// GetReflectedProperty
//------------------------------------------------------------------------------
const ReflectedProperty * ReflectionInfo::GetReflectedProperty( const AString & propertyName ) const
{
    return FindProperty( propertyName.Get() );
}

// SetProperty
//------------------------------------------------------------------------------
#define GETSET_PROPERTY( getValueType, setValueType ) \
    bool ReflectionInfo::GetProperty( void * object, const char * name, getValueType * value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (getValueType *)nullptr ) ) && !p->IsArray() ) \
        { \
            p->GetProperty( object, value ); \
            return true; \
        } \
        return false; \
    } \
    bool ReflectionInfo::SetProperty( void * object, const char * name, setValueType value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (getValueType *)nullptr ) ) && !p->IsArray() ) \
        { \
            p->SetProperty( object, value ); \
            return true; \
        } \
        return false; \
    }

GETSET_PROPERTY( float, float )
GETSET_PROPERTY( uint8_t, uint8_t )
GETSET_PROPERTY( uint16_t, uint16_t )
GETSET_PROPERTY( uint32_t, uint32_t )
GETSET_PROPERTY( uint64_t, uint64_t )
GETSET_PROPERTY( int8_t, int8_t )
GETSET_PROPERTY( int16_t, int16_t )
GETSET_PROPERTY( int32_t, int32_t )
GETSET_PROPERTY( int64_t, int64_t )
GETSET_PROPERTY( bool, bool )
GETSET_PROPERTY( AString, const AString & )

#define GETSET_PROPERTY_ARRAY( valueType ) \
    bool ReflectionInfo::GetProperty( void * object, const char * name, Array< valueType > * value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (valueType *)nullptr ) ) && p->IsArray() ) \
        { \
            p->GetProperty( object, value ); \
            return true; \
        } \
        return false; \
    } \
    bool ReflectionInfo::SetProperty( void * object, const char * name, const Array< valueType > & value ) const \
    { \
        const ReflectedProperty * p = FindProperty( name ); \
        if ( p && ( p->GetType() == GetPropertyType( (valueType *)nullptr ) ) && p->IsArray() ) \
        { \
            p->SetProperty( object, value ); \
            return true; \
        } \
        return false; \
    }

GETSET_PROPERTY_ARRAY( AString )

#undef GETSET_PROPERTY
#undef GETSET_PROPERTY_ARRAY

// SetTypeName
//------------------------------------------------------------------------------
void ReflectionInfo::SetTypeName( const char * typeName )
{
    m_TypeName = typeName;
    m_TypeNameCRC = xxHash::Calc32( typeName, AString::StrLen( typeName ) );
}

// HasMetaDataInternal
//------------------------------------------------------------------------------
const IMetaData * ReflectionInfo::HasMetaDataInternal( const ReflectionInfo * ri ) const
{
    const IMetaData * m = m_MetaDataChain;
    while ( m )
    {
        if ( m->GetReflectionInfoV() == ri )
        {
            return m;
        }
        m = m->GetNext();
    }
    return m_SuperClass ? m_SuperClass->HasMetaDataInternal( ri ) : nullptr;
}

// AddProperty
//------------------------------------------------------------------------------
void ReflectionInfo::AddProperty( uint32_t offset, const char * memberName, PropertyType type )
{
    ReflectedProperty * r = new ReflectedProperty( memberName, offset, type, false );
    m_Properties.Append( r );
}

// AddPropertyStruct
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyStruct( uint32_t offset, const char * memberName, const ReflectionInfo * structInfo )
{
    ReflectedPropertyStruct * r = new ReflectedPropertyStruct( memberName, offset, structInfo );
    m_Properties.Append( r );
}

// AddPropertyArray
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyArray( uint32_t offset, const char * memberName, PropertyType type )
{
    ReflectedProperty * r = new ReflectedProperty( memberName, offset, type, true );
    m_Properties.Append( r );
}

// AddPropertyArrayOfStruct
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyArrayOfStruct( uint32_t offset, const char * memberName, const ReflectionInfo * structInfo )
{
    ReflectedPropertyStruct * r = new ReflectedPropertyStruct( memberName, offset, structInfo, true );
    m_Properties.Append( r );
}

// AddMetaData
//------------------------------------------------------------------------------
void ReflectionInfo::AddMetaData( const MetaNone & /*metaNone*/ )
{
    ASSERT( m_MetaDataChain == nullptr );
}

// AddMetaData
//------------------------------------------------------------------------------
void ReflectionInfo::AddMetaData( IMetaData & metaDataChain )
{
    ASSERT( m_MetaDataChain == nullptr );
    m_MetaDataChain = &metaDataChain;
}

// AddPropertyMetaData
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyMetaData( const MetaNone & /*metaNone*/ )
{
}

// AddPropertyMetaData
//------------------------------------------------------------------------------
void ReflectionInfo::AddPropertyMetaData( IMetaData & metaDataChain )
{
    m_Properties.Top()->AddMetaData( &metaDataChain );
}

//  FindProperty
//------------------------------------------------------------------------------
const ReflectedProperty * ReflectionInfo::FindProperty( const char * name ) const
{
    const uint32_t nameCRC = xxHash::Calc32( name, AString::StrLen( name ) );
    return FindPropertyRecurse( nameCRC );
}

//  FindProperty
//------------------------------------------------------------------------------
const ReflectedProperty * ReflectionInfo::FindPropertyRecurse( uint32_t nameCRC ) const
{
    for ( const ReflectedProperty * property : m_Properties )
    {
        if ( property->GetNameCRC() == nameCRC )
        {
            return property;
        }
    }
    if ( m_SuperClass )
    {
        return m_SuperClass->FindPropertyRecurse( nameCRC );
    }
    return nullptr;
}

// SetArraySize
//------------------------------------------------------------------------------
void ReflectionInfo::SetArraySize( void * array, size_t size ) const
{
    ASSERT( IsStruct() );
    ASSERT( !IsAbstract() );
    SetArraySizeV( array, size );
}

// SetArraySizeV
//------------------------------------------------------------------------------
/*virtual*/ void ReflectionInfo::SetArraySizeV( void * /*array*/, size_t /*size*/ ) const
{
    ASSERT( false ); // Should be implemented by derived class!
}

//------------------------------------------------------------------------------
