// TestChainedMemoryStream.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/TestGroup.h"

// Core
#include "Core/FileIO/ChainedMemoryStream.h"
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
TEST_GROUP( TestChainedMemoryStream, TestGroupTest )
{
public:
};

//------------------------------------------------------------------------------
TEST_CASE( TestChainedMemoryStream, Empty )
{
    ChainedMemoryStream ms( 1024 );
    TEST_ASSERT( ms.GetFileSize() == 0 );
    TEST_ASSERT( ms.Tell() == 0 );
    TEST_ASSERT( ms.GetNumPages() == 0 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestChainedMemoryStream, WriteOnePartialPage )
{
    ChainedMemoryStream ms( 1024 );
    const uint64_t value = 0;
    TEST_ASSERT( ms.Write( value ) );
    TEST_ASSERT( ms.GetFileSize() == sizeof( value ) );
    TEST_ASSERT( ms.Tell() == ms.GetFileSize() );
    TEST_ASSERT( ms.GetNumPages() == 1 );
}

//------------------------------------------------------------------------------
TEST_CASE( TestChainedMemoryStream, WriteOneFullPage )
{
    // Perform several writes that to fill exactly one page
    ChainedMemoryStream ms( 32 );
    TEST_ASSERT( ms.WriteBuffer( "ABCD1234", 8 ) == 8 );
    TEST_ASSERT( ms.WriteBuffer( "zxcvbnm,zxcvbnm,", 16 ) == 16 );
    TEST_ASSERT( ms.WriteBuffer( "QWERTYUI", 8 ) == 8 );

    // Should have one page of the exact size written
    TEST_ASSERT( ms.GetFileSize() == 32 );
    TEST_ASSERT( ms.Tell() == ms.GetFileSize() );
    TEST_ASSERT( ms.GetNumPages() == 1 );
    uint32_t dataSize = 0;
    const char * const data = ms.GetPage( 0, dataSize );
    TEST_ASSERT( data );
    TEST_ASSERT( dataSize == ms.GetFileSize() );

    // Check content of page is as expected
    AStackString buffer( data, ( data + dataSize ) );
    TEST_ASSERT( buffer == "ABCD1234zxcvbnm,zxcvbnm,QWERTYUI" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestChainedMemoryStream, WriteSeveralFullPages )
{
    // Perform several writes that to fill several pages
    ChainedMemoryStream ms( 16 );
    TEST_ASSERT( ms.WriteBuffer( "ABCDEFGH", 8 ) == 8 );
    TEST_ASSERT( ms.WriteBuffer( "xxxx----", 8 ) == 8 );
    TEST_ASSERT( ms.WriteBuffer( "12345678", 8 ) == 8 );
    TEST_ASSERT( ms.WriteBuffer( "qwertyui", 8 ) == 8 );

    // Should have several pages
    TEST_ASSERT( ms.GetFileSize() == 32 );
    TEST_ASSERT( ms.Tell() == ms.GetFileSize() );
    TEST_ASSERT( ms.GetNumPages() == 2 );
    AStackString buffer;
    for ( uint32_t i = 0; i < ms.GetNumPages(); ++i )
    {
        uint32_t dataSize = 0;
        const char * const data = ms.GetPage( i, dataSize );
        TEST_ASSERT( data );
        TEST_ASSERT( dataSize == 16 );
        buffer.Append( data, ( data + dataSize ) );
    }
    TEST_ASSERT( buffer.GetLength() == ms.GetFileSize() );
    TEST_ASSERT( buffer == "ABCDEFGHxxxx----12345678qwertyui" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestChainedMemoryStream, WriteAcrossPageBoundary )
{
    // Partially fill the first page
    ChainedMemoryStream ms( 32 );
    TEST_ASSERT( ms.WriteBuffer( "ABCDabcd", 8 ) == 8 );
    TEST_ASSERT( ms.GetFileSize() == 8 );

    // Write data that cannot fit in remained and will be split over 2 pages
    TEST_ASSERT( ms.WriteBuffer( "12345678901234567890123456789012", 32 ) == 32 );

    // Check state is as expected
    TEST_ASSERT( ms.GetFileSize() == 40 );
    TEST_ASSERT( ms.GetNumPages() == 2 );
    AStackString buffer;
    {
        uint32_t dataSize = 0;
        const char * const data = ms.GetPage( 0, dataSize );
        TEST_ASSERT( data );
        TEST_ASSERT( dataSize == 32 );
        buffer.Append( data, ( data + dataSize ) );
    }
    {
        uint32_t dataSize = 0;
        const char * const data = ms.GetPage( 1, dataSize );
        TEST_ASSERT( data );
        TEST_ASSERT( dataSize == 8 );
        buffer.Append( data, ( data + dataSize ) );
    }
    TEST_ASSERT( buffer.GetLength() == ms.GetFileSize() );
    TEST_ASSERT( buffer == "ABCDabcd12345678901234567890123456789012" );
}

//------------------------------------------------------------------------------
TEST_CASE( TestChainedMemoryStream, WriteLargerThanPage )
{
    // Perform a write that spans several pages
    const AStackString payload( "0123456789abcdef----QWERTYUIASDFGHJKZXCVBNM," );
    ChainedMemoryStream ms( 16 );
    TEST_ASSERT( ms.WriteBuffer( payload.Get(), payload.GetLength() ) == 44 );
    TEST_ASSERT( ms.GetFileSize() == 44 );
    TEST_ASSERT( ms.GetNumPages() == 3 );
    AStackString buffer;
    for ( uint32_t i = 0; i < ms.GetNumPages(); ++i )
    {
        uint32_t dataSize = 0;
        const char * const data = ms.GetPage( i, dataSize );
        buffer.Append( data, ( data + dataSize ) );
    }
    TEST_ASSERT( buffer == payload );
}

//------------------------------------------------------------------------------
