// ReflectedProperty.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ReflectedProperty.h"
#include "Core/Containers/Array.h"
#include "Core/Math/xxHash.h"
#include "Core/Reflection/PropertyType.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// System
#include <stdio.h>

#if defined( __APPLE__ ) || defined( __LINUX__ )
    #define sscanf_s sscanf // TODO:C Tidy this up
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
ReflectedProperty::ReflectedProperty( const char * name, uint32_t offset, PropertyType type, bool isArray )
{
    m_NameCRC = xxHash::Calc32( name, AString::StrLen( name ) );

    ASSERT( offset < MAX_OFFSET );
    m_Offset = (uint16_t)offset;
    m_Type = type;

    #if defined( REFLECTION_KEEP_STRING_NAMES )
        m_Name = name;
    #endif

    m_IsArray = isArray;

    m_MetaDataChain = nullptr;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
ReflectedProperty::~ReflectedProperty()
{
    const IMetaData * m = m_MetaDataChain;
    while ( m )
    {
        const IMetaData * next = m->GetNext();
        FDELETE m;
        m = next;
    }
}

// GetPropertySize
//------------------------------------------------------------------------------
size_t ReflectedProperty::GetPropertySize() const
{
    switch( m_Type )
    {
        case PT_NONE:       ASSERT( false ); return 0;
        case PT_FLOAT:      return sizeof( float );
        case PT_UINT8:      return sizeof( uint8_t );
        case PT_UINT16:     return sizeof( uint16_t );
        case PT_UINT32:     return sizeof( uint32_t );
        case PT_UINT64:     return sizeof( uint64_t );
        case PT_INT8:       return sizeof( int8_t );
        case PT_INT16:      return sizeof( int16_t );
        case PT_INT32:      return sizeof( int32_t );
        case PT_INT64:      return sizeof( int64_t );
        case PT_BOOL:       return sizeof( bool );
        case PT_ASTRING:    return sizeof( AString );
        case PT_STRUCT:
        {
            const ReflectedPropertyStruct * rps = static_cast< const ReflectedPropertyStruct * >( this );
            size_t structSize = rps->GetStructReflectionInfo()->GetStructSize();
            ASSERT( structSize > 0 );
            return structSize;
        }
    }

    ASSERT( false ); // Should never get here
    return 0;
}

// SetValue
//------------------------------------------------------------------------------
#define GETSET_PROPERTY( getValueType, setValueType ) \
    void ReflectedProperty::GetProperty( const void * object, getValueType * value ) const \
    { \
        ASSERT( (PropertyType)m_Type == GetPropertyType( value ) ); \
        ASSERT( !m_IsArray ); \
        ( *value ) = *(const getValueType *)( (size_t)object + m_Offset ); \
    } \
    void ReflectedProperty::SetProperty( void * object, setValueType value ) const \
    { \
        ASSERT( (PropertyType)m_Type == GetPropertyType( &value ) ); \
        ASSERT( !m_IsArray ); \
        *( (getValueType *)( (size_t)object + m_Offset ) ) = value; \
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
    void ReflectedProperty::GetProperty( const void * object, Array< valueType > * value ) const \
    { \
        ASSERT( (PropertyType)m_Type == GetPropertyType( ( valueType *)nullptr ) ); \
        ASSERT( m_IsArray ); \
        ( *value ) = *(const Array< valueType > *)( (size_t)object + m_Offset ); \
    } \
    void ReflectedProperty::SetProperty( void * object, const Array< valueType > & value ) const \
    { \
        ASSERT( (PropertyType)m_Type == GetPropertyType( ( valueType *)nullptr ) ); \
        ASSERT( m_IsArray ); \
        *( (Array< valueType > *)( (size_t)object + m_Offset ) ) = value; \
    }

GETSET_PROPERTY_ARRAY( AString )

#undef GETSET_PROPERTY
#undef GETSET_PROPERTY_ARRAY

// AddMetaData
//------------------------------------------------------------------------------
void ReflectedProperty::AddMetaData( const IMetaData * metaDataChain )
{
    ASSERT( m_MetaDataChain == nullptr );
    ASSERT( metaDataChain );
    m_MetaDataChain = metaDataChain;
}

// ReflectedPropertyStruct ( CONSTRUCTOR )
//------------------------------------------------------------------------------
ReflectedPropertyStruct::ReflectedPropertyStruct( const char * name, uint32_t offset, const ReflectionInfo * structInfo, bool isArray )
    : ReflectedProperty( name, offset, PT_STRUCT, isArray )
    , m_StructReflectionInfo( structInfo )
{
}

// GetStructBase
//------------------------------------------------------------------------------
const void * ReflectedPropertyStruct::GetStructBase( const void * object ) const
{
    return (const void *)( (size_t)object + m_Offset );
}

// GetStructBase
//------------------------------------------------------------------------------
void * ReflectedPropertyStruct::GetStructBase( void * object ) const
{
    return (void *)( (size_t)object + m_Offset );
}

//
//------------------------------------------------------------------------------
size_t ReflectedPropertyStruct::GetArraySize( const void * object ) const
{
    // sanity checks
    ASSERT( IsArray() );
    ASSERT( GetType() == PT_STRUCT );

    // get the array
    const void * arrayBase = (const void *)( (size_t)object + m_Offset );
    const Array< char > * array = static_cast< const Array< char > * >( arrayBase );

    // NOTE: This assumes Array stores the size explicitly (and does not calculate it
    //       based on the element size)
    return array->GetSize();
}

//------------------------------------------------------------------------------
void ReflectedPropertyStruct::ResizeArrayOfStruct( void * object, size_t newSize ) const
{
    // sanity checks
    ASSERT( IsArray() );
    ASSERT( GetType() == PT_STRUCT );

    const ReflectionInfo * structRI = m_StructReflectionInfo;
    void * arrayBase = (void *)( (size_t)object + m_Offset );
    structRI->SetArraySize( arrayBase, newSize );
}

//------------------------------------------------------------------------------
Struct * ReflectedPropertyStruct::GetStructInArray( void * object, size_t index ) const
{
    // sanity checks
    ASSERT( IsArray() );
    ASSERT( GetType() == PT_STRUCT );
    ASSERT( index < GetArraySize( object ) );

    // get the array
    const size_t elementSize = GetPropertySize();
    const void * arrayBase = (const void *)( (size_t)object + m_Offset );
    const Array< char > * array = static_cast< const Array< char > * >( arrayBase );

    // calculate the element offset
    const size_t offset = ( index * elementSize );
    return reinterpret_cast< Struct * >( array->Begin() + offset );
}

//------------------------------------------------------------------------------
const Struct * ReflectedPropertyStruct::GetStructInArray( const void * object, size_t index ) const
{
    // sanity checks
    ASSERT( IsArray() );
    ASSERT( GetType() == PT_STRUCT );
    ASSERT( index < GetArraySize( object ) );

    // get the array
    const size_t elementSize = GetPropertySize();
    const void * arrayBase = (const void *)( (size_t)object + m_Offset );
    const Array< char > * array = static_cast< const Array< char > * >( arrayBase );

    // calculate the element offset
    const size_t offset = ( index * elementSize );
    return reinterpret_cast< Struct * >( array->Begin() + offset );
}

//------------------------------------------------------------------------------
