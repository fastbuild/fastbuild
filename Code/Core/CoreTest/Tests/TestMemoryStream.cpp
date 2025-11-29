// TestMemoryStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/FileIO/MemoryStream.h"
#include "Core/Strings/AStackString.h"

// TestMemoryStream
//------------------------------------------------------------------------------
class TestMemoryStream : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void InitiallyEmpty() const;
    void InitialBuffer() const;
    void Reset() const;
    void MoveConstructor() const;
    void MoveAssignment() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestMemoryStream )
    REGISTER_TEST( Empty )
    REGISTER_TEST( InitiallyEmpty )
    REGISTER_TEST( InitialBuffer )
    REGISTER_TEST( Reset )
    REGISTER_TEST( MoveConstructor )
    REGISTER_TEST( MoveAssignment )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestMemoryStream::Empty() const
{
    MemoryStream ms;
    TEST_ASSERT( ms.GetData() == nullptr );
    TEST_ASSERT( ms.GetSize() == 0 );
    TEST_ASSERT( ms.Tell() == 0 );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
}

//------------------------------------------------------------------------------
void TestMemoryStream::InitiallyEmpty() const
{
    // Start empty and write ensure growth from no initial buffer is handled
    // correctly
    MemoryStream ms;
    AStackString buffer( "hello" );
    TEST_ASSERT( ms.WriteBuffer( buffer.Get(), buffer.GetLength() ) == buffer.GetLength() );
    TEST_ASSERT( ms.GetData() != nullptr );
    TEST_ASSERT( ms.GetSize() == buffer.GetLength() );
    TEST_ASSERT( ms.Tell() == ms.GetSize() );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
}

// InitialBuffer
//------------------------------------------------------------------------------
void TestMemoryStream::InitialBuffer() const
{
    MemoryStream ms( 1024 );
    TEST_ASSERT( ms.GetData() != nullptr );
    TEST_ASSERT( ms.GetSize() == 0 );
    TEST_ASSERT( ms.Tell() == 0 );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
}

// Reset
//------------------------------------------------------------------------------
void TestMemoryStream::Reset() const
{
    MemoryStream ms;
    ms.Write( static_cast<uint32_t>( 1u ) );

    TEST_ASSERT( ms.GetSize() == sizeof( uint32_t ) );
    TEST_ASSERT( ms.GetSize() == ms.Tell() );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );

    ms.Reset();

    TEST_ASSERT( ms.GetSize() == 0 );
    TEST_ASSERT( ms.GetSize() == ms.Tell() );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
}

// MoveConstructor
//------------------------------------------------------------------------------
void TestMemoryStream::MoveConstructor() const
{
    MemoryStream ms1;
    ms1.Write( static_cast<uint32_t>( 1u ) );

    MemoryStream ms2( Move( ms1 ) );

    // ms2 now owns the memory
    TEST_ASSERT( ms2.GetData() != nullptr );
    TEST_ASSERT( ms2.GetSize() == sizeof( uint32_t ) );
    TEST_ASSERT( ms2.GetSize() == ms2.Tell() );
    TEST_ASSERT( ms2.GetSize() == ms2.GetFileSize() );

    // ms has been cleared
    PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use after move is intentional
    TEST_ASSERT( ms1.GetData() == nullptr );
    TEST_ASSERT( ms1.GetSize() == 0 );
    TEST_ASSERT( ms1.GetSize() == ms1.Tell() );
    TEST_ASSERT( ms1.GetSize() == ms1.GetFileSize() );
    PRAGMA_DISABLE_POP_MSVC
}

// MoveAssignment
//------------------------------------------------------------------------------
void TestMemoryStream::MoveAssignment() const
{
    MemoryStream ms1;
    ms1.Write( static_cast<uint32_t>( 1u ) );

    MemoryStream ms2;

    ms2 = Move( ms1 );

    // ms2 now owns the memory
    TEST_ASSERT( ms2.GetData() != nullptr );
    TEST_ASSERT( ms2.GetSize() == sizeof( uint32_t ) );
    TEST_ASSERT( ms2.GetSize() == ms2.Tell() );
    TEST_ASSERT( ms2.GetSize() == ms2.GetFileSize() );

    // ms has been cleared
    PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use after move is intentional
    TEST_ASSERT( ms1.GetData() == nullptr );
    TEST_ASSERT( ms1.GetSize() == 0 );
    TEST_ASSERT( ms1.GetSize() == ms1.Tell() );
    TEST_ASSERT( ms1.GetSize() == ms1.GetFileSize() );
    PRAGMA_DISABLE_POP_MSVC
}

//------------------------------------------------------------------------------
