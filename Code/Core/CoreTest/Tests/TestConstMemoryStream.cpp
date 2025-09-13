// TestConstMemoryStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/MemoryStream.h"

// TestConstMemoryStream
//------------------------------------------------------------------------------
class TestConstMemoryStream : public TestGroup
{
private:
    DECLARE_TESTS

    void Empty() const;
    void MemoryNotOwned() const;
    void MemoryOwned() const;
    void MoveConstructor() const;
    void MoveAssignment() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestConstMemoryStream )
    REGISTER_TEST( Empty )
    REGISTER_TEST( MemoryNotOwned )
    REGISTER_TEST( MemoryOwned )
    REGISTER_TEST( MoveConstructor )
    REGISTER_TEST( MoveAssignment )
REGISTER_TESTS_END

// Empty
//------------------------------------------------------------------------------
void TestConstMemoryStream::Empty() const
{
    ConstMemoryStream ms;
    TEST_ASSERT( ms.GetData() == nullptr );
    TEST_ASSERT( ms.GetSize() == 0 );
    TEST_ASSERT( ms.Tell() == 0 );
    TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
}

// MemoryNotOwned
//------------------------------------------------------------------------------
void TestConstMemoryStream::MemoryNotOwned() const
{
    // Buffer on stack
    const size_t bufferSize = 1024;
    const char buffer[ bufferSize ] = { 0 };

    // Wrap via constructor
    {
        ConstMemoryStream ms( buffer, bufferSize );
        TEST_ASSERT( ms.GetData() != nullptr );
        TEST_ASSERT( ms.GetSize() == bufferSize );
        TEST_ASSERT( ms.Tell() == 0 );
        TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
    }

    // Wrap via Replace()
    {
        ConstMemoryStream ms;
        ms.Replace( buffer, bufferSize, false ); // Do not take ownership
        TEST_ASSERT( ms.GetData() != nullptr );
        TEST_ASSERT( ms.GetSize() == bufferSize );
        TEST_ASSERT( ms.Tell() == 0 );
        TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
    }
}

// MemoryOwned
//------------------------------------------------------------------------------
void TestConstMemoryStream::MemoryOwned() const
{
    // Buffer on heap
    const size_t bufferSize = 1024;
    const void * buffer = ALLOC( bufferSize );

    // Wrap via Replace()
    {
        ConstMemoryStream ms;
        ms.Replace( buffer, bufferSize, true ); // Take ownership
        TEST_ASSERT( ms.GetData() != nullptr );
        TEST_ASSERT( ms.GetSize() == bufferSize );
        TEST_ASSERT( ms.Tell() == 0 );
        TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
    }
}

// MoveConstructor
//------------------------------------------------------------------------------
void TestConstMemoryStream::MoveConstructor() const
{
    // From ConstMemoryStream
    {
        // Buffer on stack
        const size_t bufferSize = 1024;
        const char buffer[ bufferSize ] = { 0 };

        ConstMemoryStream ms( buffer, bufferSize );
        ConstMemoryStream ms2( Move( ms ) );

        // First stream should now be empty
        PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use after move is intentional
        TEST_ASSERT( ms.GetData() == nullptr );
        TEST_ASSERT( ms.GetSize() == 0 );
        TEST_ASSERT( ms.Tell() == 0 );
        TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
        PRAGMA_DISABLE_POP_MSVC

        // Second stream contains data
        TEST_ASSERT( ms2.GetData() != nullptr );
        TEST_ASSERT( ms2.GetSize() == bufferSize );
        TEST_ASSERT( ms2.Tell() == 0 );
        TEST_ASSERT( ms2.GetSize() == ms2.GetFileSize() );
    }

    // From MemoryStream
    {
        // MemoryStream with buffer
        const size_t bufferSize = 1024;
        MemoryStream ms;
        for ( size_t i = 0; i < bufferSize; ++i )
        {
            ms.Write( static_cast<uint8_t>( 'x' ) );
        }

        ConstMemoryStream ms2( Move( ms ) );

        // First stream should now be empty
        PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use after move is intentional
        TEST_ASSERT( ms.GetData() == nullptr );
        TEST_ASSERT( ms.GetSize() == 0 );
        TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
        PRAGMA_DISABLE_POP_MSVC

        // Second stream contains data
        TEST_ASSERT( ms2.GetData() != nullptr );
        TEST_ASSERT( ms2.GetSize() == bufferSize );
        TEST_ASSERT( ms2.Tell() == 0 );
        TEST_ASSERT( ms2.GetSize() == ms2.GetFileSize() );
    }
}

// MoveAssignment
//------------------------------------------------------------------------------
void TestConstMemoryStream::MoveAssignment() const
{
    // Buffer on stack (unowned)
    {
        const size_t bufferSize = 1024;
        const char buffer[ bufferSize ] = { 0 };

        ConstMemoryStream ms( buffer, bufferSize );
        ConstMemoryStream ms2;
        ms2 = Move( ms );

        // First stream should now be empty
        PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use after move is intentional
        TEST_ASSERT( ms.GetData() == nullptr );
        TEST_ASSERT( ms.GetSize() == 0 );
        TEST_ASSERT( ms.Tell() == 0 );
        TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
        PRAGMA_DISABLE_POP_MSVC

        // Second stream contains data
        TEST_ASSERT( ms2.GetData() != nullptr );
        TEST_ASSERT( ms2.GetSize() == bufferSize );
        TEST_ASSERT( ms2.Tell() == 0 );
        TEST_ASSERT( ms2.GetSize() == ms2.GetFileSize() );
    }

    // Buffer on heap (owned)
    {
        const size_t bufferSize = 1024;
        const void * buffer = ALLOC( bufferSize );

        ConstMemoryStream ms;
        ms.Replace( buffer, bufferSize, true ); // Take ownership
        ConstMemoryStream ms2;
        ms2 = Move( ms );

        // First stream should now be empty
        PRAGMA_DISABLE_PUSH_MSVC( 26800 ) // Use after move is intentional
        TEST_ASSERT( ms.GetData() == nullptr );
        TEST_ASSERT( ms.GetSize() == 0 );
        TEST_ASSERT( ms.Tell() == 0 );
        TEST_ASSERT( ms.GetSize() == ms.GetFileSize() );
        PRAGMA_DISABLE_POP_MSVC

        // Second stream contains data
        TEST_ASSERT( ms2.GetData() != nullptr );
        TEST_ASSERT( ms2.GetSize() == bufferSize );
        TEST_ASSERT( ms2.Tell() == 0 );
        TEST_ASSERT( ms2.GetSize() == ms2.GetFileSize() );
    }
}

//------------------------------------------------------------------------------
