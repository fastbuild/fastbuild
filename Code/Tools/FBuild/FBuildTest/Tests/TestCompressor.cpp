// TestCompressor.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"

#include "Core/Containers/AutoPtr.h"
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
    REGISTER_TEST( CompressSimple );
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
    d.Decompress( compressedMem );
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
    AutoPtr< void > data;
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

    // compress the data to obtain size
    Compressor comp;
    comp.Compress( data.Get(), dataSize );
    size_t compressedSize = comp.GetResultSize();
    AutoPtr< char > compressedData( (char *)ALLOC( compressedSize ) );
    memcpy( compressedData.Get(), comp.GetResult(), compressedSize );
    float compressedPerc = ( (float)compressedSize / (float)dataSize ) * 100.0f;
    OUTPUT( "Compressed Size: %u (%2.1f%% of original)\n", (uint32_t)compressedSize, compressedPerc );

    // decompress to check we get original data back
    Compressor decomp;
    decomp.Decompress( compressedData.Get() );
    size_t uncompressedSize = decomp.GetResultSize();
    TEST_ASSERT( uncompressedSize == dataSize );
    for ( size_t i=0; i<uncompressedSize; ++i )
    {
        TEST_ASSERT( ( (char *)data.Get() )[ i ] == ( (char *)decomp.GetResult() )[ i ] );
    }

    // speed checks
    //--------------
    const float TIME_TO_REPEAT( 0.3f );

    // compress the data several times to get more stable throughput value
    Timer t;
    uint32_t numRepeats( 0 );
    while ( t.GetElapsed() < TIME_TO_REPEAT )
    {
        Compressor c;
        c.Compress( data.Get(), dataSize );
        TEST_ASSERT( c.GetResultSize() == compressedSize );
        ++numRepeats;
    }
    float compressTimeTaken = t.GetElapsed();
    double compressThroughputMBs = ( ( (double)dataSize / 1024.0 * (double)numRepeats ) / compressTimeTaken ) / 1024.0;
    OUTPUT( "     Comp Speed: %2.1f MB/s - %2.3fs (%u repeats)\n", (float)compressThroughputMBs, compressTimeTaken, numRepeats );

    // decompress the data
    Timer t2;
    numRepeats = 0;
    while ( t2.GetElapsed() < TIME_TO_REPEAT )
    {
        Compressor d;
        d.Decompress( compressedData.Get() );
        TEST_ASSERT( d.GetResultSize() == dataSize );
        ++numRepeats;
    }
    float decompressTimeTaken = t2.GetElapsed();
    double decompressThroughputMBs = ( ( (double)dataSize / 1024.0 * (double)numRepeats ) / decompressTimeTaken ) / 1024.0;
    OUTPUT( "   Decomp Speed: %2.1f MB/s - %2.3fs (%u repeats)\n", (float)decompressThroughputMBs, decompressTimeTaken, numRepeats );

    // time memcpy to compare with
    Timer t0;
    numRepeats = 0;
    while ( t0.GetElapsed() < TIME_TO_REPEAT )
    {
        char * mem = (char *)ALLOC( dataSize );
        memcpy( mem, data.Get(), dataSize );
        FREE( mem );
        ++numRepeats;
    }
    float memcpyTimeTaken = t0.GetElapsed();
    double memcpyThroughputMBs = ( ( (double)dataSize / 1024.0 * (double)numRepeats ) / memcpyTimeTaken ) / 1024.0;
    OUTPUT( "   MemCpy Speed: %2.1f MB/s - %2.3fs (%u repeats)\n", (float)memcpyThroughputMBs, memcpyTimeTaken, numRepeats );
}

// TestHeaderValidity
//------------------------------------------------------------------------------
void TestCompressor::TestHeaderValidity() const
{
    AutoPtr< uint32_t > buffer( (uint32_t *)ALLOC( 1024 ) );
    memset( buffer.Get(), 0, 1024 );
    Compressor c;
    uint32_t * data = (uint32_t *)buffer.Get();

    // uncompressed buffer of 0 length is valid
    TEST_ASSERT( c.IsValidData( buffer.Get(), 12 ) );

    // compressed buffer of 0 length is valid
    data[0] = 1;
    TEST_ASSERT( c.IsValidData( buffer.Get(), 12 ) );

    // compressed data
    data[1] = 32; // uncompressed
    data[2] = 8; // compressed
    TEST_ASSERT( c.IsValidData( buffer.Get(), 20 ) );

    // INVALID data - data too small
    TEST_ASSERT( c.IsValidData( buffer.Get(), 4 ) == false );

    // INVALID data - compressed bigger than uncompressed
    data[1] = 8; // uncompressed
    data[2] = 32; // compressed
    TEST_ASSERT( c.IsValidData( buffer.Get(), 44 ) == false );
}

//------------------------------------------------------------------------------
