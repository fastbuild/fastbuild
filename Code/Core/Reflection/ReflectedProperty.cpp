// ReflectedProperty.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "ReflectedProperty.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Ref.h"
#include "Core/Containers/WeakRef.h"
#include "Core/Math/CRC32.h"
#include "Core/Math/Mat44.h"
#include "Core/Math/Vec2.h"
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"
#include "Core/Reflection/Object.h"
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
	m_NameCRC = CRC32::Calc( name, AString::StrLen( name ) );

	ASSERT( offset < MAX_OFFSET );
	m_Offset = offset;
	m_Type = (uint32_t)type;

	#if defined( REFLECTION_KEEP_STRING_NAMES )
		m_Name = name;
	#endif

	m_IsArray = isArray;

	m_MetaDataChain = nullptr;
}

// GetPropertySize
//------------------------------------------------------------------------------
size_t ReflectedProperty::GetPropertySize() const
{
	switch( m_Type )
	{
		case PT_NONE:		ASSERT( false ); return 0;
		case PT_FLOAT:		return sizeof( float );
		case PT_UINT8:		return sizeof( uint8_t );
		case PT_UINT16:		return sizeof( uint16_t );
		case PT_UINT32:		return sizeof( uint32_t );
		case PT_UINT64:		return sizeof( uint64_t );
		case PT_INT8:		return sizeof( int8_t );
		case PT_INT16:		return sizeof( int16_t );
		case PT_INT32:		return sizeof( int32_t );
		case PT_INT64:		return sizeof( int64_t );
		case PT_BOOL:		return sizeof( bool );
		case PT_ASTRING:	return sizeof( AString );
		case PT_VEC2:		return sizeof( Vec2 );
		case PT_VEC3:		return sizeof( Vec3 );
		case PT_VEC4:		return sizeof( Vec4 );
		case PT_MAT44:		return sizeof( Mat44 );
		case PT_STRUCT:
		{
			const ReflectedPropertyStruct * rps = static_cast< const ReflectedPropertyStruct * >( this );
			size_t structSize = rps->GetStructReflectionInfo()->GetStructSize();
			ASSERT( structSize > 0 );
			return structSize;
		}
		case PT_REF:		return sizeof( Ref< Object > );
		case PT_WEAKREF:	return sizeof( WeakRef< Object > );
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
GETSET_PROPERTY( Vec2, const Vec2 & )
GETSET_PROPERTY( Vec3, const Vec3 & )
GETSET_PROPERTY( Vec4, const Vec4 & )
GETSET_PROPERTY( Mat44, const Mat44 & )
GETSET_PROPERTY( Ref< RefObject >, const Ref< RefObject > & )
GETSET_PROPERTY( WeakRef< Object >, const WeakRef< Object > & )

#define GETSET_PROPERTY_ARRAY( valueType ) \
	void ReflectedProperty::GetProperty( const void * object, const Array< valueType > * & value ) const \
	{ \
		ASSERT( (PropertyType)m_Type == GetPropertyType( ( valueType *)nullptr ) ); \
		ASSERT( m_IsArray ); \
		value = (const Array< valueType > *)( (size_t)object + m_Offset ); \
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

// ToString
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
	void ReflectedProperty::ToString( const void * object, AString & buffer ) const
	{
		switch ( m_Type )
		{
			case PT_FLOAT:
			{
				float f;
				GetProperty( object, &f );
				buffer.Format( "%1.1f", f ); // TODO: Find a good format specifier
				return;
			}
			case PT_UINT8:
			{ 
				uint8_t u;
				GetProperty( object, &u );
				buffer.Format( "%u", u );
				return;
			}
			case PT_UINT16:
			{ 
				uint16_t u;
				GetProperty( object, &u );
				buffer.Format( "%u", u );
				return;
			}
			case PT_UINT32:
			{ 
				uint32_t u;
				GetProperty( object, &u );
				buffer.Format( "%u", u );
				return;
			}
			case PT_UINT64:
			{ 
				uint64_t u;
				GetProperty( object, &u );
				buffer.Format( "%llu", u );
				return;
			}
			case PT_INT8:
			{
				int8_t i;
				GetProperty( object, &i );
				buffer.Format( "%i", i );
				return;
			}
			case PT_INT16:
			{
				int16_t i;
				GetProperty( object, &i );
				buffer.Format( "%i", i );
				return;
			}
			case PT_INT32:
			{
				int32_t i;
				GetProperty( object, &i );
				buffer.Format( "%i", i );
				return;
			}
			case PT_INT64:
			{ 
				int64_t i;
				GetProperty( object, &i );
				buffer.Format( "%lld", i );
				return;
			}
			case PT_BOOL:
			{
				bool b;
				GetProperty( object, &b );
				buffer.Format( "%s", b ? "true" : "false" );
				return;
			}
			case PT_ASTRING:
			{ 
				AStackString<> str;
				GetProperty( object, &str );
				buffer.Format( "%s", str.Get() ); // TODO: Think about escaping
				break;
			}
			case PT_VEC2:
			{
				Vec2 v;
				GetProperty( object, &v );
				buffer.Format( "%.1f, %.1f", v.x, v.y ); // TODO: Find a good format specifier
				return;
			}
			case PT_VEC3:
			{
				Vec3 v;
				GetProperty( object, &v );
				buffer.Format( "%.1f, %.1f, %.1f", v.x, v.y, v.z ); // TODO: Find a good format specifier
				return;
			}
			case PT_VEC4:
			{
				Vec4 v;
				GetProperty( object, &v );
				buffer.Format( "%.1f, %.1f, %.1f, %.1f", v.x, v.y, v.z, v.w ); // TODO: Find a good format specifier
				return;
			}
			case PT_MAT44:
			{
				Mat44 m;
				GetProperty( object, &m );
				buffer.Format( "%.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %.1f", // TODO: Find a good format specifier
						m.col0.x, m.col0.y, m.col0.z, m.col0.z, 
						m.col1.x, m.col1.y, m.col1.z, m.col1.z, 
						m.col2.x, m.col2.y, m.col2.z, m.col2.z, 
						m.col3.x, m.col3.y, m.col3.z, m.col3.z );
				return;
			}
			case PT_WEAKREF:
			{
				WeakRef< Object > w;
				GetProperty( object, &w );
				if ( w.Get() == nullptr )
				{
					buffer.Format( "null" );
				}
				else
				{
					w->GetScopedName( buffer );
				}
				return;
			}
			case PT_REF:
			case PT_STRUCT:
			{
				ASSERT( false ); // Unsupported
			}
		}
	}
#endif

// FromString
//------------------------------------------------------------------------------
bool ReflectedProperty::FromString( const AString & buffer, float * value )
{
	return ( sscanf_s( buffer.Get(), "%f", value ) == 1 );
}
bool ReflectedProperty::FromString( const AString & buffer, uint8_t * value )
{
	uint32_t u32;
	if ( sscanf_s( buffer.Get(), "%u", &u32 ) == 1 )
	{
		if ( u32 <= 255 )
		{
			*value = (uint8_t)u32;
			return true;
		}
	}
	return false;
}
bool ReflectedProperty::FromString( const AString & buffer, uint16_t * value )
{
	uint32_t u32;
	if ( sscanf_s( buffer.Get(), "%u", &u32 ) == 1 )
	{
		if ( u32 <= 65535 )
		{
			*value = (uint16_t)u32;
			return true;
		}
	}
	return false;
}
bool ReflectedProperty::FromString( const AString & buffer, uint32_t * value )
{
	return ( sscanf_s( buffer.Get(), "%u", value ) == 1 );
}
bool ReflectedProperty::FromString( const AString & buffer, uint64_t * value )
{
    #if defined( __LINUX__ )
        return ( sscanf_s( buffer.Get(), "%lu", value ) == 1 );
    #else
        return ( sscanf_s( buffer.Get(), "%llu", value ) == 1 );
    #endif
}
bool ReflectedProperty::FromString( const AString & buffer, int8_t * value )
{
	int32_t i32;
	if ( sscanf_s( buffer.Get(), "%i", &i32 ) == 1 )
	{
		if ( ( i32 >= -128 ) && ( i32 <= 127 ) )
		{
			*value = (int8_t)i32;
			return true;
		}
	}
	return false;
}
bool ReflectedProperty::FromString( const AString & buffer, int16_t * value )
{
	int32_t i32;
	if ( sscanf_s( buffer.Get(), "%i", &i32 ) == 1 )
	{
		if ( ( i32 >= -32768 ) && ( i32 <= 32767 ) )
		{
			*value = (int16_t)i32;
			return true;
		}
	}
	return false;
}
bool ReflectedProperty::FromString( const AString & buffer, int32_t * value )
{
	return ( sscanf_s( buffer.Get(), "%i", value ) == 1 );
}
bool ReflectedProperty::FromString( const AString & buffer, int64_t * value )
{
    #if defined( __LINUX__ )
        return ( sscanf_s( buffer.Get(), "%ld", value ) == 1 );
    #else
        return ( sscanf_s( buffer.Get(), "%lld", value ) == 1 );
    #endif
}
bool ReflectedProperty::FromString( const AString & buffer, bool * value )
{
	if ( AString::StrNCmpI( buffer.Get(), "true", 4 ) == 0 )
	{
		*value = true;
		return true;
	}
	if ( AString::StrNCmpI( buffer.Get(), "false", 5 ) == 0 )
	{
		*value = false;
		return true;
	}
	return false;
}
bool ReflectedProperty::FromString( const AString & buffer, AString * value )
{
	*value = buffer;
	return true;
}
bool ReflectedProperty::FromString( const AString & buffer, Vec2 * value )
{
	return ( sscanf_s( buffer.Get(), "%f, %f", &value->x, &value->y ) == 2 );
}
bool ReflectedProperty::FromString( const AString & buffer, Vec3 * value )
{
	return ( sscanf_s( buffer.Get(), "%f, %f, %f", &value->x, &value->y, &value->z ) == 3 );
}
bool ReflectedProperty::FromString( const AString & buffer, Vec4 * value )
{
	return ( sscanf_s( buffer.Get(),"%f, %f, %f, %f", &value->x, &value->y, &value->z, &value->w ) == 4 );
}
bool ReflectedProperty::FromString( const AString & buffer, Mat44 * value )
{
	return ( sscanf_s( buffer.Get(), "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
					&value->col0.x, &value->col0.y, &value->col0.z, &value->col0.z,
					&value->col1.x, &value->col1.y, &value->col1.z, &value->col1.z,
					&value->col2.x, &value->col2.y, &value->col2.z, &value->col2.z,
					&value->col3.x, &value->col3.y, &value->col3.z, &value->col3.z ) == 16 );
}

// TypeToTypeString
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
	/*static*/ const char * ReflectedProperty::TypeToTypeString( PropertyType type )
	{
		switch ( type )
		{
			case PT_FLOAT:		return "float"; 
			case PT_UINT8:		return "u8";
			case PT_UINT16:		return "u16";
			case PT_UINT32:		return "u32";
			case PT_UINT64:		return "u64";
			case PT_INT8:		return "i8";
			case PT_INT16:		return "i16";
			case PT_INT32:		return "i32";
			case PT_INT64:		return "i64";
			case PT_BOOL:		return "bool";
			case PT_ASTRING:	return "aStr";
			case PT_VEC2:		return "vec2";
			case PT_VEC3:		return "vec3";
			case PT_VEC4:		return "vec4";
			case PT_MAT44:		return "mat44";
			case PT_WEAKREF:	return "weakRef";
			case PT_REF:		return "ref";
			case PT_STRUCT:		return "struct";
			case PT_NONE:
			{
				ASSERT( false ); // Unsupported
			}
		}
		return "???";
	}
#endif

// AddMetaData
//------------------------------------------------------------------------------
void ReflectedProperty::AddMetaData( const IMetaData * metaDataChain )
{
	ASSERT( m_MetaDataChain == nullptr );
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

// 
//------------------------------------------------------------------------------
size_t ReflectedPropertyStruct::GetArraySize( const void * object ) const
{
	// sanity checks
	ASSERT( IsArray() );
	ASSERT( GetType() == PT_STRUCT );

	// get the array
	const size_t elementSize = GetPropertySize();
	const void * arrayBase = (const void *)( (size_t)object + m_Offset );
	const Array< char > * array = static_cast< const Array< char > * >( arrayBase );

	// calculate the size
	const size_t size = ( array->End() - array->Begin() ) / elementSize;
	return size;
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
