// ReflectionMacros.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Reflection/ReflectionInfo.h"
#include "Core/Reflection/MetaData/MetaData.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ReflectionInfo;

// Reflection
//------------------------------------------------------------------------------

// DECLARE
//------------------------------------------------------------------------------
#define REFLECT_STRUCT_DECLARE( structName ) \
    private: \
        friend class structName##_ReflectionInfo; \
        static const ReflectionInfo * s_ReflectionInfo; \
    public: \
        static const ReflectionInfo * GetReflectionInfoS();

#define REFLECT_DECLARE( className ) \
    REFLECT_STRUCT_DECLARE( className ) \
        virtual const ReflectionInfo * GetReflectionInfoV() const override;

// HELPER
//------------------------------------------------------------------------------
#define CHECK_BASE_CLASS( className, baseClass ) \
    className * c = nullptr; \
    baseClass * b = static_cast< baseClass * >( c ); (void)b;

#define ADD_METADATA( metaData ) \
    AddMetaData( metaData );

#define ADD_PROPERTY_METADATA( metaData ) \
    AddPropertyMetaData( metaData );

// BEGIN
//------------------------------------------------------------------------------
#define REFLECT_BEGIN_COMMON( className, baseClass, metaData, structSize, isAbstract ) \
    class baseClass##_ReflectionInfo; \
    extern baseClass##_ReflectionInfo g_##baseClass##_ReflectionInfo; \
    class className##_ReflectionInfo; \
    extern className##_ReflectionInfo g_##className##_ReflectionInfo; \
    const ReflectionInfo * className::s_ReflectionInfo( nullptr ); \
    const ReflectionInfo * className::GetReflectionInfoS() \
    { \
        return reinterpret_cast< const ReflectionInfo * >( &g_##className##_ReflectionInfo ); \
    } \
    class className##_ReflectionInfo : public ReflectionInfo \
    { \
    public: \
        typedef className objectType; \
        explicit className##_ReflectionInfo() \
        { \
            SetTypeName( #className ); \
            className::s_ReflectionInfo = this; \
            AddProperties(); \
            m_StructSize = structSize; \
            m_IsAbstract = isAbstract; \
            m_SuperClass = reinterpret_cast< const ReflectionInfo * >( &g_##baseClass##_ReflectionInfo ); \
            ADD_METADATA( metaData ) \
        } \
        virtual ~className##_ReflectionInfo() \
        { \
            className::s_ReflectionInfo = nullptr; \
        }

#define REFLECT_BEGIN_ABSTRACT( className, baseClass, metaData ) \
    const ReflectionInfo * className::GetReflectionInfoV() const \
    { \
        return className::GetReflectionInfoS(); \
    } \
    REFLECT_BEGIN_COMMON( className, baseClass, metaData, 0, true ) \
    void AddProperties() \
    { \
        CHECK_BASE_CLASS( className, baseClass )

#define REFLECT_BEGIN( className, baseClass, metaData ) \
    const ReflectionInfo * className::GetReflectionInfoV() const \
    { \
        return className::GetReflectionInfoS(); \
    } \
    REFLECT_BEGIN_COMMON( className, baseClass, metaData, 0, false ) \
        virtual void * Create() const override \
        { \
            return FNEW( className ); \
        } \
        void AddProperties() \
        { \
            CHECK_BASE_CLASS( className, baseClass )

#define REFLECT_STRUCT_BEGIN( structName, baseStruct, metaData ) \
    REFLECT_BEGIN_COMMON( structName, baseStruct, metaData, sizeof( structName ), false ) \
        virtual void * Create() const override \
        { \
            return FNEW( structName ); \
        } \
        virtual void SetArraySizeV( void * array, size_t size ) const override \
        { \
            Array< structName > * realArray = static_cast< Array< structName > * >( array ); \
            realArray->SetSize( size ); \
        } \
        void AddProperties() \
        { \
            CHECK_BASE_CLASS( structName, baseStruct )

#define REFLECT_STRUCT_BEGIN_ABSTRACT( structName, baseStruct, metaData ) \
    REFLECT_BEGIN_COMMON( structName, baseStruct, metaData, sizeof( structName ), false ) \
        void AddProperties() \
        { \
            CHECK_BASE_CLASS( structName, baseStruct )

#define REFLECT_STRUCT_BEGIN_BASE( structName ) \
    REFLECT_BEGIN_COMMON( structName, Struct, MetaNone(), sizeof( structName ), false ) \
        virtual void * Create() const override \
        { \
            return FNEW( structName ); \
        } \
        virtual void SetArraySizeV( void * array, size_t size ) const override \
        { \
            Array< structName > * realArray = static_cast< Array< structName > * >( array ); \
            realArray->SetSize( size ); \
        } \
        void AddProperties() \
        {

// MEMBERS
//------------------------------------------------------------------------------
#define REFLECT( member, memberName, metaData ) \
            AddProperty( &((objectType *)0)->member, memberName ); \
            ADD_PROPERTY_METADATA( metaData )

#define REFLECT_ARRAY( member, memberName, metaData ) \
            AddPropertyArray( &((objectType *)0)->member, memberName ); \
            ADD_PROPERTY_METADATA( metaData )

#define REFLECT_STRUCT( member, memberName, structType, metaData ) \
            AddPropertyStruct( &((objectType *)0)->member, memberName, structType::GetReflectionInfoS() ); \
            ADD_PROPERTY_METADATA( metaData )

#define REFLECT_ARRAY_OF_STRUCT( member, memberName, structType, metaData ) \
            AddPropertyArrayOfStruct( &((objectType *)0)->member, memberName, structType::GetReflectionInfoS() ); \
            ADD_PROPERTY_METADATA( metaData )

// END
//------------------------------------------------------------------------------
#define REFLECT_END( className ) \
        } \
    }; \
    className##_ReflectionInfo g_##className##_ReflectionInfo; \
    void className##_ReflectionInfo_Bind() \
    { \
        ReflectionInfo::BindReflection( g_##className##_ReflectionInfo ); \
    }

//------------------------------------------------------------------------------
