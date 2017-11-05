// TestHash.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/Math/CRC32.h"
#include "Core/Math/Random.h"
#include "Core/Math/xxHash.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

#include <memory.h>

// TestHash
//------------------------------------------------------------------------------
class TestHash : public UnitTest
{
private:
    DECLARE_TESTS

    void CompareHashTimes_Large() const;
    void CompareHashTimes_Small() const;
};

// Register Tests
//------------------------------------------------------------------------------
REGISTER_TESTS_BEGIN( TestHash )
    REGISTER_TEST( CompareHashTimes_Large )
    REGISTER_TEST( CompareHashTimes_Small )
REGISTER_TESTS_END

// CompareHashTimes_Large
//------------------------------------------------------------------------------
void TestHash::CompareHashTimes_Large() const
{
    // use pseudo-random (but deterministic) data
    const uint32_t seed = 0xB1234567;
    Random r( seed );

    // fill a buffer to use for tests
    const size_t dataSize( 64 * 1024 * 1024 );
    AutoPtr< uint32_t > data( (uint32_t *)ALLOC( dataSize ) );
    for ( size_t i=0; i<dataSize / sizeof( uint32_t ); ++i )
    {
        data.Get()[ i ] = r.GetRand();
    }

    // baseline - sum 64 bits
    {
        Timer t;
        uint64_t sum( 0 );
        uint64_t * it = (uint64_t *)data.Get();
        uint64_t * end = it + ( dataSize / sizeof( uint64_t ) );
        while ( it != end )
        {
            sum += *it;
            ++it;
        }
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "Sum64           : %2.3fs @ %6.3f GiB/s (sum: %016" PRIx64 ")\n", time, speed, sum );
    }


    // baseline - sum 32 bits
    {
        Timer t;
        uint32_t sum( 0 );
        uint32_t * it = data.Get();
        uint32_t * end = it + ( dataSize / sizeof( uint32_t ) );
        while ( it != end )
        {
            sum += *it;
            ++it;
        }
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "Sum32           : %2.3fs @ %6.3f GiB/s (sum: 0x%x)\n", time, speed, sum );
    }

    // xxHash32
    {
        Timer t;
        uint32_t crc = xxHash::Calc32( data.Get(), dataSize );
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "xxHash-32       : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }

    // xxHash64
    {
        Timer t;
        uint64_t crc = xxHash::Calc64( data.Get(), dataSize );
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "xxHash-64       : %2.3fs @ %6.3f GiB/s (hash: %016" PRIx64 ")\n", time, speed, crc );
    }

    // CRC32 - 8x8 slicing
    {
        Timer t;
        uint32_t crc = CRC32::Calc( data.Get(), dataSize );
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "CRC32 8x8       : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }

    // CRC32 - "standard" algorithm
    {
        Timer t;
        uint32_t crc = CRC32::Start();
        crc = CRC32::Update( crc, data.Get(), dataSize );
        crc = CRC32::Stop( crc );
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "CRC32           : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }

    // CRC32Lower
    {
        Timer t;
        uint32_t crc = CRC32::CalcLower( data.Get(), dataSize );
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "CRC32Lower      : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }
}

// CompareHashTimes_Small
//------------------------------------------------------------------------------
void TestHash::CompareHashTimes_Small() const
{
    // some different strings to hash
    Array< AString > strings( 32, true );
    strings.Append( AString( " " ) );
    strings.Append( AString( "shOrt" ) );
    strings.Append( AString( "MediumstringMediumstring123456789" ) );
    strings.Append( AString( "longstring_98274ncoif834JODhiorhmwe8r8wy48on87h8mhwejrijrdIERwurd9j,8chm8hiuorciwriowjri" ) );
    strings.Append( AString( "c:\\files\\subdir\\project\\thing\\stuff.cpp" ) );
    const size_t numStrings = strings.GetSize();
    const size_t numIterations = 102400;

    // calc datasize
    size_t dataSize( 0 );
    for ( size_t i=0; i<numStrings; ++i )
    {
        dataSize += strings[ i ].GetLength();
    }
    dataSize *= numIterations;

    // xxHash - 32
    {
        Timer t;
        uint32_t crc( 0 );
        for ( size_t j=0; j<numIterations; ++j )
        {
            for ( size_t i=0; i<numStrings; ++i )
            {
                crc += xxHash::Calc32( strings[ i ].Get(), strings[ i ].GetLength() );
            }
        }
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "xxHash-32       : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }

    // xxHash - 64
    {
        Timer t;
        uint64_t crc( 0 );
        for ( size_t j=0; j<numIterations; ++j )
        {
            for ( size_t i=0; i<numStrings; ++i )
            {
                crc += xxHash::Calc64( strings[ i ].Get(), strings[ i ].GetLength() );
            }
        }
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "xxHash-64       : %2.3fs @ %6.3f GiB/s (hash: %016" PRIx64 ")\n", time, speed, crc );
    }

    // CRC32 - 8x8 slicing
    {
        Timer t;
        uint32_t crc( 0 );
        for ( size_t j=0; j<numIterations; ++j )
        {
            for ( size_t i=0; i<numStrings; ++i )
            {
                crc += CRC32::Calc( strings[ i ].Get(), strings[ i ].GetLength() );
            }
        }
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "CRC32 8x8       : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }

    // CRC32 - "standard" algorithm
    {
        Timer t;
        uint32_t crc( 0 );
        for ( size_t j=0; j<numIterations; ++j )
        {
            for ( size_t i=0; i<numStrings; ++i )
            {
                uint32_t crc2 = CRC32::Start();
                crc2 = CRC32::Update( crc2, strings[ i ].Get(), strings[ i ].GetLength() );
                crc2 = CRC32::Stop( crc2 );
                crc += crc2;
            }
        }
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "CRC32           : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }

    // CRC32Lower
    {
        Timer t;
        uint32_t crc( 0 );
        for ( size_t j=0; j<numIterations; ++j )
        {
            for ( size_t i=0; i<numStrings; ++i )
            {
                crc += CRC32::CalcLower( strings[ i ].Get(), strings[ i ].GetLength() );
            }
        }
        float time = t.GetElapsed();
        float speed = ( (float)dataSize / (float)( 1024 * 1024 * 1024 ) ) / time;
        OUTPUT( "CRC32Lower      : %2.3fs @ %6.3f GiB/s (hash: 0x%x)\n", time, speed, crc );
    }
}

//------------------------------------------------------------------------------
