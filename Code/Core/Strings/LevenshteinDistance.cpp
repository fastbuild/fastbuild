// LevenshteinDistance.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "LevenshteinDistance.h"
#include "AString.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"

// CharEqual<>
//------------------------------------------------------------------------------
namespace
{
    template < bool _CaseSentitive >
    struct CharEqual
    {
        bool operator ()( char lhs, char rhs ) const
        {
            return lhs == rhs;
        }
    };

    template <>
    struct CharEqual< false >
    {
        bool operator ()( char lhs, char rhs ) const
        {
            lhs = ( ( lhs >= 'A' ) && ( lhs <= 'Z' ) ) ? 'a' + ( lhs - 'A' ) : lhs;
            rhs = ( ( rhs >= 'A' ) && ( rhs <= 'Z' ) ) ? 'a' + ( rhs - 'A' ) : rhs;
            return lhs == rhs;
        }
    };
}

// Levenshtein distance
// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C
//------------------------------------------------------------------------------
template < size_t _Capacity, bool _CaseSentitive >
static uint32_t LevenshteinDistanceImpl( const char * str1, uint32_t len1,
                                         const char * str2, uint32_t len2 )
{
    ASSERT( 0 == len1 || nullptr != str1 );
    ASSERT( 0 == len2 || nullptr != str2 );

    if ( 0 == len1 || nullptr == str1 ) return len2;
    if ( 0 == len2 || nullptr == str2 ) return len1;

    // swap str1 and str2 if str2 is shorter
    if ( len2 < len1 )
    {
        const char * strTmp = str1;
        const uint32_t lenTmp = len1;
        str1 = str2;
        len1 = len2;
        str2 = strTmp;
        len2 = lenTmp;
    }

    if ( len1 >= _Capacity )
    {
        ASSERT( false );
        return len2;
    }

    uint32_t column[ _Capacity ] = { 0 };

    for ( uint32_t y = 1 ; y <= len1; y++ )
        column[ y ] = y;

    for ( uint32_t x = 1 ; x <= len2; x++ )
    {
        column[ 0 ] = x;
        for ( uint32_t y = 1, lastdiag = x - 1 ; y <= len1 ; y++ )
        {
            const uint32_t olddiag = column[ y ];

            const uint32_t a = column[y] + 1;
            const uint32_t b = column[y - 1] + 1;
            const uint32_t c = lastdiag + ( CharEqual< _CaseSentitive >()( str1[y - 1], str2[x - 1] ) ? 0 : 1 );
            column[ y ] = ( ( a < b ) ? ( a < c ? a : c ) : ( b < c ? b : c ) );

            lastdiag = olddiag;
        }
    }

    return column[ len1 ];
}

// Distance
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::Distance( const char * lhs, const char * rhs )
{
    const uint32_t lhsLen = ( nullptr == lhs ) ? 0 : (uint32_t)AString::StrLen( lhs );
    const uint32_t rhsLen = ( nullptr == rhs ) ? 0 : (uint32_t)AString::StrLen( rhs );
    return ::LevenshteinDistanceImpl< 1024, true >( lhs, lhsLen, rhs, rhsLen );
}

// Distance
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::Distance( const AString & lhs, const AString & rhs )
{
    return ::LevenshteinDistanceImpl< 1024, true >( lhs.Get(), lhs.GetLength(), rhs.Get(), rhs.GetLength() );
}

// DistanceI
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::DistanceI( const char * lhs, const char * rhs )
{
    const uint32_t lhsLen = ( nullptr == lhs ) ? 0 : (uint32_t)AString::StrLen( lhs );
    const uint32_t rhsLen = ( nullptr == rhs ) ? 0 : (uint32_t)AString::StrLen( rhs );
    return ::LevenshteinDistanceImpl< 1024, false >( lhs, lhsLen, rhs, rhsLen );
}

// DistanceI
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::DistanceI( const AString & lhs, const AString & rhs )
{
    return ::LevenshteinDistanceImpl< 1024, false >( lhs.Get(), lhs.GetLength(), rhs.Get(), rhs.GetLength() );
}

//------------------------------------------------------------------------------
