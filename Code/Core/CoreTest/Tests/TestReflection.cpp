// TestAString.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include <TestFramework/TestGroup.h>

#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Reflection/MetaData/Meta_File.h"
#include "Core/Reflection/MetaData/Meta_Path.h"
#include "Core/Reflection/Object.h"
#include "Core/Reflection/ReflectedProperty.h"
#include "Core/Reflection/Struct.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// system
#include <memory.h>

// TestReflection
//------------------------------------------------------------------------------
class TestReflection : public TestGroup
{
private:
    DECLARE_TESTS

    void IsArrayProperty() const;
    void IsStruct() const;
    void GetStructType() const;
    void TestGetSet() const;
    void TestInheritance() const;
    void MetaData() const;
    void ArraySize() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestReflection )
    REGISTER_TEST( IsArrayProperty )
    REGISTER_TEST( IsStruct )
    REGISTER_TEST( GetStructType )
    REGISTER_TEST( TestGetSet )
    REGISTER_TEST( TestInheritance )
    REGISTER_TEST( MetaData )
    REGISTER_TEST( ArraySize )
REGISTER_TESTS_END

// TestStruct
//------------------------------------------------------------------------------
struct TestStruct : public Struct
{
    REFLECT_STRUCT_DECLARE( TestStruct )
public:
    TestStruct()
        : m_MyInt( 851 )
    {
    }

    uint32_t m_MyInt;
};

REFLECT_STRUCT_BEGIN_BASE( TestStruct )
    REFLECT( m_MyInt )
REFLECT_END( TestStruct )

// TestObject
//------------------------------------------------------------------------------
class TestObject : public Object
{
    REFLECT_DECLARE( TestObject )
public:
    TestObject()
        : m_Float( 0.0f )
        , m_UInt8( 0 )
        , m_UInt16( 0 )
        , m_UInt32( 0 )
        , m_UInt64( 0 )
        , m_Int8( 0 )
        , m_Int16( 0 )
        , m_Int32( 0 )
        , m_Int64( 0 )
        , m_Bool( false )
        , m_AString( "" )
    {
    }

    void PopulateWithTestData()
    {
        m_Float = 99.0f;
        m_UInt8 = 42;
        m_UInt16 = 9901;
        m_UInt32 = 1000000;
        m_UInt64 = 100000000000000;
        m_Int8 = 43;
        m_Int16 = 9202;
        m_Int32 = 1000001;
        m_Int64 = 100000000000001;
        m_Bool = true;
        m_AString = "Test string.";

        m_FloatArray.Append( 111.0f );
        m_FloatArray.Append( 222.0f );
        m_FloatArray.Append( 333.0f );

        m_StructArray.SetSize( 3 );
    }

private: // ensure reflection can set private members
    friend class TestReflection;

    float m_Float;
    uint8_t m_UInt8;
    uint16_t m_UInt16;
    uint32_t m_UInt32;
    uint64_t m_UInt64;
    int8_t m_Int8;
    int16_t m_Int16;
    int32_t m_Int32;
    int64_t m_Int64;
    bool m_Bool;
    AString m_AString;

    TestStruct m_TestStruct;

    Array<float> m_FloatArray;

    Array<TestStruct> m_StructArray;
};

REFLECT_BEGIN( TestObject, Object )
    REFLECT( m_Float )
    REFLECT( m_UInt8 )
    REFLECT( m_UInt16 )
    REFLECT( m_UInt32 )
    REFLECT( m_UInt64 )
    REFLECT( m_Int8 )
    REFLECT( m_Int16 )
    REFLECT( m_Int32 )
    REFLECT( m_Int64 )
    REFLECT( m_Bool )
    REFLECT( m_AString )
    REFLECT( m_TestStruct )
    REFLECT( m_FloatArray )
    REFLECT( m_StructArray )
REFLECT_END( TestObject )

//------------------------------------------------------------------------------
void TestReflection::IsArrayProperty() const
{
    const float singleFloat = 0.0f;
    const TestStruct singleStruct;
    const Array<float> arrayFloats;
    const Array<TestStruct> arrayStructs;

    static_assert( ::IsArrayProperty( &singleFloat ) == false );
    static_assert( ::IsArrayProperty( &singleStruct ) == false );
    static_assert( ::IsArrayProperty( &arrayFloats ) == true );
    static_assert( ::IsArrayProperty( &arrayStructs ) == true );
}

//------------------------------------------------------------------------------
void TestReflection::IsStruct() const
{
    // Structures (alone or in Arrays)
    const TestStruct singleStruct;
    const Array<TestStruct> arrayStructs;
    static_assert( ::IsStruct( &singleStruct ) == true );
    static_assert( ::IsStruct( &arrayStructs ) == true );

    // Non-Struct single types
    const float singleFloat = 0.0f;
    const AString singleString;
    static_assert( ::IsStruct( &singleFloat ) == false );
    static_assert( ::IsStruct( &singleString ) == false );

    // Non-Struct arrays
    const Array<float> arrayFloats;
    const Array<AString> arrayStrings;
    static_assert( ::IsStruct( &arrayFloats ) == false );
    static_assert( ::IsStruct( &arrayStrings ) == false );
}

//------------------------------------------------------------------------------
void TestReflection::GetStructType() const
{
    // Anything that derives from Struct should return valid ReflectionInfo
    const TestStruct singleStruct;
    const Array<TestStruct> arrayStructs;
    TEST_ASSERT( ::GetStructType( &singleStruct ) == TestStruct::GetReflectionInfoS() );
    TEST_ASSERT( ::GetStructType( &arrayStructs ) == TestStruct::GetReflectionInfoS() );

    // Non-Struct types should return nullptr
    const float singleFloat = 0.0f;
    const Array<float> arrayFloats;
    TEST_ASSERT( ::GetStructType( &singleFloat ) == nullptr );
    TEST_ASSERT( ::GetStructType( &arrayFloats ) == nullptr );
}

// TestGetSet
//------------------------------------------------------------------------------
void TestReflection::TestGetSet() const
{
    TestObject o;
    const ReflectionInfo * info = o.GetReflectionInfoV();

#define CHECK( name, member, type, value ) \
    { \
        TEST_ASSERT( info->SetProperty( &o, name, (type)value ) ); \
        TEST_ASSERT( o.member == value ); \
        type v; \
        TEST_ASSERT( info->GetProperty( &o, name, &v ) ); \
        TEST_ASSERT( v == value ); \
    }

    CHECK( "Float", m_Float, float, 3.0f )
    CHECK( "UInt8", m_UInt8, uint8_t, 7 )
    CHECK( "UInt16", m_UInt16, uint16_t, 10101 )
    CHECK( "UInt32", m_UInt32, uint32_t, 0x12345678 )
    CHECK( "UInt64", m_UInt64, uint64_t, 0x8765432112345678 )
    CHECK( "Int8", m_Int8, int8_t, 7 )
    CHECK( "Int16", m_Int16, int16_t, 10101 )
    CHECK( "Int32", m_Int32, int32_t, 0x12345678 )
    CHECK( "Int64", m_Int64, int64_t, 0x7765432112345678 )
    CHECK( "Bool", m_Bool, bool, true )
    CHECK( "AString", m_AString, AString, AString( "hello" ) )

#undef CHECK
}

// TestInheritance
//------------------------------------------------------------------------------
class BaseClass : public Object
{
    REFLECT_DECLARE( BaseClass )
public:
    BaseClass()
        : m_A( -1 )
    {
    }
    int m_A;
};

class DerivedClass : public BaseClass
{
    REFLECT_DECLARE( DerivedClass )
public:
    DerivedClass()
        : m_B( -1 )
    {
    }
    int m_B;
};

REFLECT_BEGIN( BaseClass, Object )
    REFLECT( m_A )
REFLECT_END( BaseClass )

REFLECT_BEGIN( DerivedClass, BaseClass )
    REFLECT( m_B )
REFLECT_END( DerivedClass )

void TestReflection::TestInheritance() const
{
    // Create an object with inheritance
    DerivedClass obj;

    // Set property on class
    const ReflectionInfo * ri = obj.GetReflectionInfoV();
    const int bValue = 200;
    TEST_ASSERT( ri->SetProperty( &obj, "B", bValue ) );
    int bValueGet = 0;
    TEST_ASSERT( ri->GetProperty( &obj, "B", &bValueGet ) );
    TEST_ASSERT( bValue == bValueGet );

    // Set property on base class
    const int aValue = 100;
    TEST_ASSERT( ri->SetProperty( &obj, "A", aValue ) );
    int aValueGet = 0;
    TEST_ASSERT( ri->GetProperty( &obj, "A", &aValueGet ) );
    TEST_ASSERT( aValue == aValueGet );
}

// MetaData
//------------------------------------------------------------------------------
class ObjectWithMetaData : public Object
{
    REFLECT_DECLARE( ObjectWithMetaData )
public:
    uint32_t m_Property = 0;
};

REFLECT_BEGIN( ObjectWithMetaData, Object, MetaFile() + MetaPath() )
    REFLECT( m_Property, MetaFile() + MetaPath() )
REFLECT_END( ObjectWithMetaData )

void TestReflection::MetaData() const
{
    ObjectWithMetaData obj;
    const ReflectionInfo * ri = obj.GetReflectionInfoV();

    // Check all MetaData is present on object
    TEST_ASSERT( ri->HasMetaData<Meta_File>() );
    TEST_ASSERT( ri->HasMetaData<Meta_Path>() );

    // Check all MetaData is present on property
    const ReflectedProperty * rp = ri->GetReflectedProperty( AStackString( "Property" ) );
    TEST_ASSERT( rp->HasMetaData<Meta_File>() );
    TEST_ASSERT( rp->HasMetaData<Meta_Path>() );
}

// ArraySize
//------------------------------------------------------------------------------
void TestReflection::ArraySize() const
{
    // The reflection system makes assumptions about Array's implementation
    // (how it records the array size) which need to be updated if changed.
    // This test should fail if our assumptions are invalidated
    TestObject o;
    const ReflectionInfo * ri = TestObject::GetReflectionInfoS();
    const ReflectedProperty * rp = ri->GetReflectedProperty( AStackString( "StructArray" ) );
    const ReflectedPropertyStruct * rps = (const ReflectedPropertyStruct *)rp;
    TEST_ASSERT( rps->GetArraySize( &o ) == 0 );
    o.m_StructArray.SetSize( 4 );
    TEST_ASSERT( rps->GetArraySize( &o ) == 4 );
}

//------------------------------------------------------------------------------
