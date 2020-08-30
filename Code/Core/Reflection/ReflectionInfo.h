// ReflectionInfo.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Env/Types.h"
#include "Core/Reflection/PropertyType.h"
#include "Core/Reflection/ReflectionIter.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class IMetaData;
class MetaNone;
class Object;
class ReflectionInfo;
class ReflectedProperty;
class Struct;

// ReflectionInfo
//------------------------------------------------------------------------------
class ReflectionInfo
{
public:
    explicit ReflectionInfo();
    virtual ~ReflectionInfo();

    inline const ReflectionInfo * GetSuperClass() const { return m_SuperClass; }

    inline const char * GetTypeName() const { return m_TypeName; }
    inline bool IsStruct() const { return ( m_StructSize > 0 ); }
    inline bool IsObject() const { return ( m_StructSize == 0 ); }
    inline bool IsAbstract() const { return m_IsAbstract; }
    inline size_t GetStructSize() const { return m_StructSize; }

    ReflectionIter Begin() const;
    ReflectionIter End() const;

    const ReflectedProperty & GetReflectedProperty( uint32_t index ) const;
    const ReflectedProperty * GetReflectedProperty( const AString & propertyName ) const;

    void SetArraySize( void * array, size_t size ) const;

    #define GETSET_PROPERTY( getValueType, setValueType ) \
        bool GetProperty( void * object, const char * name, getValueType * value ) const; \
        bool SetProperty( void * object, const char * name, setValueType value ) const;

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
        bool GetProperty( void * object, const char * name, Array< valueType > * value ) const; \
        bool SetProperty( void * object, const char * name, const Array< valueType > & value ) const;

    GETSET_PROPERTY_ARRAY( AString )

    #undef GETSET_PROPERTY
    #undef GETSET_PROPERTY_ARRAY

    template < class T >
    const T * HasMetaData() const
    {
        const ReflectionInfo * ri = T::GetReflectionInfoS();
        return (const T *)HasMetaDataInternal( ri );
    }

protected:
    const IMetaData * HasMetaDataInternal( const ReflectionInfo * ri ) const;

    void SetTypeName( const char * typeName );

    // basic types
    void AddProperty( uint32_t offset, const char * memberName, PropertyType type );

    // struct
    void AddPropertyStruct( uint32_t offset, const char * memberName, const ReflectionInfo * structInfo );

    // array
    void AddPropertyArray( uint32_t offset, const char * memberName, PropertyType type );

    // array of struct
    void AddPropertyArrayOfStruct( uint32_t offset, const char * memberName, const ReflectionInfo * structInfo );

    void AddMetaData( const MetaNone & metaNone );
    void AddMetaData( IMetaData & metaDataChain );
    void AddPropertyMetaData( const MetaNone & metaNone );
    void AddPropertyMetaData( IMetaData & metaDataChain );

    const ReflectedProperty * FindProperty( const char * name ) const;
    const ReflectedProperty * FindPropertyRecurse( uint32_t nameCRC ) const;

    virtual void SetArraySizeV( void * array, size_t size ) const;

    uint32_t m_TypeNameCRC;
    Array< ReflectedProperty * > m_Properties;
    const ReflectionInfo * m_SuperClass;
    const ReflectionInfo * m_Next;
    const char * m_TypeName;
    bool m_IsAbstract;
    uint32_t m_StructSize;
    IMetaData * m_MetaDataChain;
};

//------------------------------------------------------------------------------
