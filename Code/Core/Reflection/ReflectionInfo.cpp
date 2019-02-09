// ReflectionInfo.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "ReflectionInfo.h"
#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/CRC32.h"
#include "Core/Process/Process.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Strings/AString.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// System
#include <memory.h>

// Static Data
//------------------------------------------------------------------------------
/*static*/ ReflectionInfo * ReflectionInfo::s_FirstReflectionInfo( nullptr );

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
    auto end = m_Properties.End();
    for ( auto it = m_Properties.Begin(); it != end; ++it )
    {
        delete *it;
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

// BindReflection
//------------------------------------------------------------------------------
/*static*/ void ReflectionInfo::BindReflection( ReflectionInfo & reflectionInfo )
{
    ASSERT( reflectionInfo.m_Next == nullptr );
    reflectionInfo.m_Next = s_FirstReflectionInfo;
    s_FirstReflectionInfo = &reflectionInfo;
}

// SetTypeName
//------------------------------------------------------------------------------
void ReflectionInfo::SetTypeName( const char * typeName )
{
    m_TypeName = typeName;
    m_TypeNameCRC = CRC32::Calc( typeName, AString::StrLen( typeName ) );
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
    const uint32_t nameCRC = CRC32::Calc( name, AString::StrLen( name ) );
    return FindPropertyRecurse( nameCRC );
}

//  FindProperty
//------------------------------------------------------------------------------
const ReflectedProperty * ReflectionInfo::FindPropertyRecurse( uint32_t nameCRC ) const
{
    auto end = m_Properties.End();
    for ( auto it = m_Properties.Begin(); it != end; ++it )
    {
        if ( ( *it )->GetNameCRC() == nameCRC )
        {
            return *it;
        }
    }
    if ( m_SuperClass )
    {
        return m_SuperClass->FindPropertyRecurse( nameCRC );
    }
    return nullptr;
}

// CreateObject
//------------------------------------------------------------------------------
/*static*/ Object * ReflectionInfo::CreateObject( const AString & objectType )
{
    const uint32_t objectTypeCRC = CRC32::Calc( objectType );
    const ReflectionInfo * ri = s_FirstReflectionInfo;
    while ( ri )
    {
        if ( objectTypeCRC == ri->m_TypeNameCRC )
        {
            return ri->CreateObject();
        }
        ri = ri->m_Next;
    }
    return nullptr;
}

// CreateStruct
//------------------------------------------------------------------------------
/*static*/ Struct * ReflectionInfo::CreateStruct( const AString & structType )
{
    const uint32_t objectTypeCRC = CRC32::Calc( structType );
    const ReflectionInfo * ri = s_FirstReflectionInfo;
    while ( ri )
    {
        if ( objectTypeCRC == ri->m_TypeNameCRC )
        {
            return ri->CreateStruct();
        }
        ri = ri->m_Next;
    }
    return nullptr;
}

// CreateObject
//------------------------------------------------------------------------------
Object * ReflectionInfo::CreateObject() const
{
    ASSERT( IsObject() );
    ASSERT( !IsAbstract() );
    return (Object *)Create();
}

// CreateStruct
//------------------------------------------------------------------------------
Struct * ReflectionInfo::CreateStruct() const
{
    ASSERT( IsStruct() );
    ASSERT( !IsAbstract() );
    return (Struct *)Create();
}

// SetArraySize
//------------------------------------------------------------------------------
void ReflectionInfo::SetArraySize( void * array, size_t size ) const
{
    ASSERT( IsStruct() );
    ASSERT( !IsAbstract() );
    SetArraySizeV( array, size );
}

// Create
//------------------------------------------------------------------------------
/*virtual*/ void * ReflectionInfo::Create() const
{
    ASSERT( false ); // Should be implemented by derived class!
    return nullptr;
}

// SetArraySizeV
//------------------------------------------------------------------------------
/*virtual*/ void ReflectionInfo::SetArraySizeV( void * UNUSED( array ), size_t UNUSED( size ) ) const
{
    ASSERT( false ); // Should be implemented by derived class!
}

//------------------------------------------------------------------------------
