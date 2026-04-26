// TestMemoryStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/FileIO/MemoryStream.h"
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestMemoryStream, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestMemoryStream, Empty )
{
    MemoryStream ms;
    TEST_ASSERT( ms.GetData() == nullptr );
    TEST_ASSERT( ms.GetSize() == 0 );
    TEST_ASSERT( ms.Tell() == 0 );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
}

//------------------------------------------------------------------------------
TEST_CASE( TestMemoryStream, InitiallyEmpty )
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

//------------------------------------------------------------------------------
TEST_CASE( TestMemoryStream, InitialBuffer )
{
    MemoryStream ms( 1024 );
    TEST_ASSERT( ms.GetData() != nullptr );
    TEST_ASSERT( ms.GetSize() == 0 );
    TEST_ASSERT( ms.Tell() == 0 );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
}

//------------------------------------------------------------------------------
TEST_CASE( TestMemoryStream, Reset )
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

//------------------------------------------------------------------------------
TEST_CASE( TestMemoryStream, MoveConstructor )
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

//------------------------------------------------------------------------------
TEST_CASE( TestMemoryStream, MoveAssignment )
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
