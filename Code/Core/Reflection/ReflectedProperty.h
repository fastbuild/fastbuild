// ReflectedProperty.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Reflection/MetaData/MetaDataInterface.h"
#include "Core/Reflection/PropertyType.h"
#include "Core/Reflection/ReflectionSettings.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class IMetaData;
class Object;
class ReflectionInfo;
class Struct;
template< class T > class Array;

// ReflectedProperty
//------------------------------------------------------------------------------
class ReflectedProperty
{
public:
    ReflectedProperty( const char * name, uint32_t offset, PropertyType type, bool isArray );
    ~ReflectedProperty();

    inline uint32_t GetNameCRC() const { return m_NameCRC; }
    inline PropertyType GetType() const { return (PropertyType)m_Type; }
    inline uint32_t GetOffset() const { return m_Offset; }
    size_t GetPropertySize() const;
    inline bool IsArray() const { return m_IsArray; }
    #if defined( REFLECTION_KEEP_STRING_NAMES )
        inline const char * GetName() const { return m_Name; }
    #endif

    template<class T>
    const T * GetPtrToProperty( const void * base ) const
    {
        ASSERT( IsArray() == false );
        const T * ptr = (T *)( (size_t)base + m_Offset );
        ASSERT( GetPropertyType( (T *)nullptr ) == GetType() );
        return ptr;
    }

    template<class T>
    T * GetPtrToProperty( void * base ) const
    {
        ASSERT( IsArray() == false );
        T * ptr = (T *)( (size_t)base + m_Offset );
        ASSERT( GetPropertyType( (T *)nullptr ) == GetType() );
        return ptr;
    }

    template<class T>
    const Array< T > * GetPtrToArray( const void * base ) const
    {
        ASSERT( IsArray() );
        const Array< T > * ptr = (const Array< T > *)( (size_t)base + m_Offset );
        ASSERT( GetPropertyType( (T *)nullptr ) == GetType() );
        return ptr;
    }

    template<class T>
    Array< T > * GetPtrToArray( void * base ) const
    {
        ASSERT( IsArray() );
        Array< T > * ptr = (Array< T > *)( (size_t)base + m_Offset );
        ASSERT( GetPropertyType( (T *)nullptr ) == GetType() );
        return ptr;
    }

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

    #define GETSET_PROPERTY_ARRAY( valueType ) \
        void GetProperty( const void * object, Array< valueType > * value ) const; \
        void SetProperty( void * object, const Array< valueType > & value ) const;

    GETSET_PROPERTY_ARRAY( AString )

    #undef GETSET_PROPERTY
    #undef GETSET_PROPERTY_ARRAY

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

    uint32_t        m_NameCRC;
    uint16_t        m_Offset; // validated by MAX_OFFSET
    PropertyType    m_Type;
    bool            m_IsArray;

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
    void *       GetStructBase( void * object ) const;

    // arrayOfStruct manipulation
    size_t      GetArraySize( const void * object ) const;
    void        ResizeArrayOfStruct( void * object, size_t newSize ) const;
    Struct *    GetStructInArray( void * object, size_t index ) const;
    const Struct *  GetStructInArray( const void * object, size_t index ) const;

    inline const ReflectionInfo * GetStructReflectionInfo() const { return m_StructReflectionInfo; }
private:
    const ReflectionInfo * m_StructReflectionInfo;
};

//------------------------------------------------------------------------------
