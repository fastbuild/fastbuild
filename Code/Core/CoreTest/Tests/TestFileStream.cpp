// TestFileStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// TestFileStream
//------------------------------------------------------------------------------
class TestFileStream : public UnitTest
{
private:
    DECLARE_TESTS

    void WriteOnly() const;
    void ReadOnly() const;

    // Helpers
    mutable uint32_t m_TempFileId = 0;
    void GenerateTempFileName( AString & outTempFileName ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestFileStream )
    REGISTER_TEST( WriteOnly )
    REGISTER_TEST( ReadOnly )
REGISTER_TESTS_END

// WriteOnly
//------------------------------------------------------------------------------
void TestFileStream::WriteOnly() const
{
    AStackString<> fileName;
    GenerateTempFileName( fileName );

    // Open a file for writing
    FileStream f;
    TEST_ASSERT( f.IsOpen() == false );
    TEST_ASSERT( f.Open( fileName.Get(), FileStream::WRITE_ONLY ) == true );

    // Sanity check initial state
    TEST_ASSERT( f.IsOpen() == true );
    TEST_ASSERT( f.GetFileSize() == 0 );
    TEST_ASSERT( f.Tell() == 0 );

    // Write some data
    const AStackString<> data( "Some Data To Store In A File" );
    TEST_ASSERT( f.WriteBuffer( data.Get(), data.GetLength() ) == data.GetLength() );

    // Sanity check new state
    TEST_ASSERT( f.IsOpen() == true );
    TEST_ASSERT( f.GetFileSize() == data.GetLength() );
    TEST_ASSERT( f.Tell() == data.GetLength() );

    // Seek
    TEST_ASSERT( f.Seek( 0 ) );
    TEST_ASSERT( f.Tell() == 0 );
    TEST_ASSERT( f.Seek( data.GetLength() ) );
    TEST_ASSERT( f.Tell() == data.GetLength() );

    // Close file
    f.Close();
    TEST_ASSERT( f.IsOpen() == false );

    // Clean up
    TEST_ASSERT( FileIO::FileDelete( fileName.Get() ) );
}

// ReadOnly
//------------------------------------------------------------------------------
void TestFileStream::ReadOnly() const
{
    AStackString<> fileName;
    GenerateTempFileName( fileName );

    const AStackString<> data( "Some Data To Store In A File" );

    // Create a file and put some data in it
    {
        FileStream f;
        TEST_ASSERT( f.Open( fileName.Get(), FileStream::WRITE_ONLY ) == true );
        TEST_ASSERT( f.WriteBuffer( data.Get(), data.GetLength() ) == data.GetLength() );
    }

    // Open for read
    {
        FileStream f;
        TEST_ASSERT( f.Open( fileName.Get(), FileStream::READ_ONLY ) == true );

        // Sanity check initial state
        TEST_ASSERT( f.IsOpen() == true );
        TEST_ASSERT( f.GetFileSize() == data.GetLength() );
        TEST_ASSERT( f.Tell() == 0 );

        // Read
        AStackString<> buffer;
        buffer.SetLength( data.GetLength() );
        TEST_ASSERT( f.ReadBuffer( buffer.Get(), data.GetLength() ) == data.GetLength() );
        TEST_ASSERT( data == buffer );
        TEST_ASSERT( f.Tell() == data.GetLength() );
    }

    // Clean up
    TEST_ASSERT( FileIO::FileDelete( fileName.Get() ) );
}

// GenerateTempFileName
//------------------------------------------------------------------------------
void TestFileStream::GenerateTempFileName( AString & outTempFileName ) const
{
    // Get system temp folder
    VERIFY( FileIO::GetTempDir( outTempFileName ) );

    // add process unique identifier
    outTempFileName.AppendFormat( "TestFileStream.%u.%u", Process::GetCurrentId(), m_TempFileId++ );
}

//------------------------------------------------------------------------------
