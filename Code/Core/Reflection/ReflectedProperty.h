// ReflectedProperty.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_REFLECTION_REFLECTEDPROPERTY_H
#define CORE_REFLECTION_REFLECTEDPROPERTY_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Reflection/ReflectionSettings.h"
#include "Core/Reflection/PropertyType.h"
#include "Core/Reflection/MetaData/MetaDataInterface.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class IMetaData;
class Mat44;
class Object;
class ReflectionInfo;
class RefObject;
class Struct;
class Vec2;
class Vec3;
class Vec4;
template< class T > class Array;

// ReflectedProperty
//------------------------------------------------------------------------------
class ReflectedProperty
{
public:
	ReflectedProperty( const char * name, uint32_t offset, PropertyType type, bool isArray );

	inline uint32_t GetNameCRC() const { return m_NameCRC; }
	inline PropertyType GetType() const { return (PropertyType)m_Type; }
	inline uint32_t GetOffset() const { return m_Offset; }
	size_t GetPropertySize() const;
	inline bool IsArray() const { return m_IsArray; }
	#if defined( REFLECTION_KEEP_STRING_NAMES )
		inline const char * GetName() const { return m_Name; }
	#endif

	#define GETSET_PROPERTY( getValueType, setValueType ) \
		void GetProperty( const void * object, getValueType * value ) const; \
		void SetProperty( void * object, setValueType value ) const;

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
		void GetProperty( const void * object, const Array< valueType > * & value ) const; \
		void SetProperty( void * object, const Array< valueType > & value ) const;

	GETSET_PROPERTY_ARRAY( AString )

	#undef GETSET_PROPERTY
	#undef GETSET_PROPERTY_ARRAY

    #if defined( __WINDOWS__ )
        inline const char * GetTypeString() const { return TypeToTypeString( GetType() ); }
        void ToString( const void * object, AString & buffer ) const;
    #endif
	static bool FromString( const AString & buffer, float * value );
	static bool FromString( const AString & buffer, uint8_t * value );
	static bool FromString( const AString & buffer, uint16_t * value );
	static bool FromString( const AString & buffer, uint32_t * value );
	static bool FromString( const AString & buffer, uint64_t * value );
	static bool FromString( const AString & buffer, int8_t * value );
	static bool FromString( const AString & buffer, int16_t * value );
	static bool FromString( const AString & buffer, int32_t * value );
	static bool FromString( const AString & buffer, int64_t * value );
	static bool FromString( const AString & buffer, bool * value );
	static bool FromString( const AString & buffer, AString * value );
	static bool FromString( const AString & buffer, Vec2 * value );
	static bool FromString( const AString & buffer, Vec3 * value );
	static bool FromString( const AString & buffer, Vec4 * value );
	static bool FromString( const AString & buffer, Mat44 * value );
	static const char * TypeToTypeString( PropertyType type );

	void AddMetaData( const IMetaData * metaDataChain );

	template < class T >
	const T * HasMetaData() const
	{
		const IMetaData * m = m_MetaDataChain;
		while ( m )
		{
			const T * m2 = ::DynamicCast< const T >( m );
			if ( m2 )
			{
				return m2;
			}
			m = m->GetNext();
		}
		return nullptr;
	}
protected:
	enum { MAX_OFFSET = ( 1 << 16 ) };

	uint32_t m_NameCRC;
	uint32_t m_Offset:16; // validated by MAX_OFFSET
	uint32_t m_Type:8;
	uint32_t m_IsArray:1;
	uint32_t m_Unused:7;

	#if defined( REFLECTION_KEEP_STRING_NAMES )
		const char * m_Name;
	#endif

	const IMetaData * m_MetaDataChain;
};

// ReflectedPropertyStruct
//------------------------------------------------------------------------------
class ReflectedPropertyStruct : public ReflectedProperty
{
public:
	explicit ReflectedPropertyStruct( const char * name, uint32_t offset, const ReflectionInfo * structInfo, bool isArray = false );

	const void * GetStructBase( const void * object ) const;

	// arrayOfStruct manipulation
	size_t		GetArraySize( const void * object ) const;
	void		ResizeArrayOfStruct( void * object, size_t newSize ) const;
	Struct *	GetStructInArray( void * object, size_t index ) const;

	inline const ReflectionInfo * GetStructReflectionInfo() const { return m_StructReflectionInfo; }
private:
	const ReflectionInfo * m_StructReflectionInfo; 
};

//------------------------------------------------------------------------------
#endif // CORE_REFLECTION_REFLECTEDPROPERTY_H
