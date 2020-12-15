// TestCompressor.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"

// Core
#include "Core/Containers/UniquePtr.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

#include <memory.h>

// TestCompressor
//------------------------------------------------------------------------------
class TestCompressor : public FBuildTest
{
private:
    DECLARE_TESTS

    void CompressSimple() const;
    void CompressPreprocessedFile() const;
    void CompressObjFile() const;
    void TestHeaderValidity() const;

    void CompressSimpleHelper( const char * data,
                               size_t size,
                               size_t expectedCompressedSize,
                               bool shouldCompress ) const;
    void CompressHelper( const char * fileName ) const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestCompressor )
    REGISTER_TEST( CompressSimple )
    REGISTER_TEST( CompressPreprocessedFile )
    REGISTER_TEST( CompressObjFile )
    REGISTER_TEST( TestHeaderValidity )
REGISTER_TESTS_END

// CompressSimple
//------------------------------------------------------------------------------
void TestCompressor::CompressSimple() const
{
    CompressSimpleHelper( "AAAAAAAA",
                          8,
                          20,
                          false );

    CompressSimpleHelper( "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
                          32,
                          23,
                          true );

    CompressSimpleHelper( "ABCDEFGH",
                          8,
                          20,
                          false );

    // a more representative piece of data
    const char * testData = "#include \"a.cpp\"\r\n#include \"b.cpp\"\r\n#include \"b.cpp\"\r\n";
    CompressSimpleHelper( testData, AString::StrLen( testData ), 0, true );

    // check for internal worst case checks
    CompressSimpleHelper( "A", 1, 0, false );
}

// CompressSimpleHelper
//------------------------------------------------------------------------------
void TestCompressor::CompressSimpleHelper( const char * data,
                                           size_t size,
                                           size_t expectedCompressedSize,
                                           bool shouldCompress ) const
{
    // raw input strings may not be aligned on Linux/OSX, so copy
    // them to achieve our required alignment
    char * alignedData = FNEW( char[ size ] );
    memcpy( alignedData, data, size );
    data = alignedData;

    // compress
    Compressor c;
    const bool compressed = c.Compress( data, size );
    TEST_ASSERT( compressed == shouldCompress );
    const size_t compressedSize = c.GetResultSize();
    if ( expectedCompressedSize > 0 )
    {
        TEST_ASSERT( compressedSize == expectedCompressedSize );
    }
    void const * compressedMem = c.GetResult();

    // decompress
    Compressor d;
    TEST_ASSERT( d.Decompress( compressedMem ) );
    const size_t decompressedSize = d.GetResultSize();
    TEST_ASSERT( decompressedSize == size );
    TEST_ASSERT( memcmp( data, d.GetResult(), size ) == 0 );

    FDELETE_ARRAY( alignedData );
}

// CompressPreprocessedFile
//------------------------------------------------------------------------------
void TestCompressor::CompressPreprocessedFile() const
{
    CompressHelper( "Tools/FBuild/FBuildTest/Data/TestCompressor/TestPreprocessedFile.ii" );
}

//------------------------------------------------------------------------------
void TestCompressor::CompressObjFile() const
{
    CompressHelper( "Tools/FBuild/FBuildTest/Data/TestCompressor/TestObjFile.o" );
}

// CompressHelper
//------------------------------------------------------------------------------
void TestCompressor::CompressHelper( const char * fileName ) const
{
    // read some test data into a file
    UniquePtr< void > data;
    size_t dataSize;
    {
        FileStream fs;
        TEST_ASSERT( fs.Open( fileName ) );
        dataSize = (size_t)fs.GetFileSize();
        data = (char *)ALLOC( dataSize );
        TEST_ASSERT( (uint32_t)fs.Read( data.Get(), dataSize ) == dataSize );
    }

    OUTPUT( "File           : %s\n", fileName );
    OUTPUT( "Size           : %u\n", (uint32_t)dataSize );

    OUTPUT( "        Compression             Decompression\n" );
    OUTPUT( "Level | Time (ms)  MB/s  Ratio | Time (ms)  MB/s\n" );
    OUTPUT( "------------------------------------------------\n" );

    // Compress at various compression levels
    const int32_t compressionLevels[] =
    { 
        0,                                          // Disabled
        -256, -128, -64, -32, -16, -8, -4, -2, -1,  // LZ4
        1, 3, 6, 9, 12                              // LZ4 HC
    };

    for ( int32_t compressionLevel : compressionLevels )
    {
        // compress/decompress the data several times to get more stable throughput value
        const uint32_t numRepeats = 4; // Increase to get more consistent numbers
        double compressTimeTaken = 0.0;
        double decompressTimeTaken = 0.0;
        uint64_t compressedSize = 0;

        // Compression speed
        UniquePtr< Compressor, DeleteDeletor > c;
        for ( uint32_t i = 0; i < numRepeats; ++i )
        {
            // Compress
            c = FNEW( Compressor );
            Timer t;
            c.Get()->Compress( data.Get(), dataSize, compressionLevel );
            compressedSize = c.Get()->GetResultSize();
            compressTimeTaken += (double)t.GetElapsedMS();
        }

        // Decompression speed
        for ( uint32_t i = 0; i < numRepeats; ++i )
        {
            // Decompress
            Timer t2;
            Compressor d;
            TEST_ASSERT( d.Decompress( c.Get()->GetResult() ) );
            TEST_ASSERT( d.GetResultSize() == dataSize );
            decompressTimeTaken += (double)t2.GetElapsedMS();

            // Sanity check decompression returns original results
            if ( i == 0 )
            {
                TEST_ASSERT( memcmp( data.Get(), d.GetResult(), dataSize ) == 0 );
            }
        }

        const double compressThroughputMBs    = ( ( (double)dataSize * (double)numRepeats ) / ( compressTimeTaken / 1000.0 ) ) / (double)MEGABYTE;
        const double decompressThroughputMBs  = ( ( (double)dataSize * (double)numRepeats ) / ( decompressTimeTaken / 1000.0 ) ) / (double)MEGABYTE;
        const double ratio = ( (double)dataSize / (double)compressedSize );

        OUTPUT( "%-5i | %8.3f %7.1f %5.2f | %8.3f %7.1f\n", compressionLevel,
                                                            ( compressTimeTaken / numRepeats ), compressThroughputMBs, (double)ratio,
                                                            ( decompressTimeTaken / numRepeats ), decompressThroughputMBs );
    }
    OUTPUT( "------------------------------------------------\n" );
}

// TestHeaderValidity
//------------------------------------------------------------------------------
void TestCompressor::TestHeaderValidity() const
{
    UniquePtr< uint32_t > buffer( (uint32_t *)ALLOC( 1024 ) );
    memset( buffer.Get(), 0, 1024 );
    Compressor c;
    uint32_t * data = (uint32_t *)buffer.Get();

    // uncompressed buffer of 0 length is valid
    TEST_ASSERT( c.IsValidData( buffer.Get(), 12 ) );

    // compressed buffer of 0 length is valid
    data[ 0 ] = 1;
    TEST_ASSERT( c.IsValidData( buffer.Get(), 12 ) );

    // compressed data
    data[ 1 ] = 32; // uncompressed
    data[ 2 ] = 8;  // compressed
    TEST_ASSERT( c.IsValidData( buffer.Get(), 20 ) );

    // INVALID data - data too small
    TEST_ASSERT( c.IsValidData( buffer.Get(), 4 ) == false );

    // INVALID data - compressed bigger than uncompressed
    data[ 1 ] = 8;  // uncompressed
    data[ 2 ] = 32; // compressed
    TEST_ASSERT( c.IsValidData( buffer.Get(), 44 ) == false );
}

//------------------------------------------------------------------------------
