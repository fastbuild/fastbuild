// LevenshteinDistance.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

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
// https://github.com/gitster/git/blob/master/levenshtein.c
//------------------------------------------------------------------------------
/*
 * This function implements the Damerau-Levenshtein algorithm to
 * calculate a distance between strings.
 *
 * Basically, it says how many letters need to be swapped, substituted,
 * deleted from, or added to string1, at least, to get string2.
 *
 * The idea is to build a distance matrix for the substrings of both
 * strings.  To avoid a large space complexity, only the last three rows
 * are kept in memory (if swaps had the same or higher cost as one deletion
 * plus one insertion, only two rows would be needed).
 *
 * At any stage, "i + 1" denotes the length of the current substring of
 * string1 that the distance is calculated for.
 *
 * row2 holds the current row, row1 the previous row (i.e. for the substring
 * of string1 of length "i"), and row0 the row before that.
 *
 * In other words, at the start of the big loop, row2[j + 1] contains the
 * Damerau-Levenshtein distance between the substring of string1 of length
 * "i" and the substring of string2 of length "j + 1".
 *
 * All the big loop does is determine the partial minimum-cost paths.
 *
 * It does so by calculating the costs of the path ending in characters
 * i (in string1) and j (in string2), respectively, given that the last
 * operation is a substitution, a swap, a deletion, or an insertion.
 *
 * This implementation allows the costs to be weighted:
 *
 * - w (as in "sWap")
 * - s (as in "Substitution")
 * - a (for insertion, AKA "Add")
 * - d (as in "Deletion")
 *
 * Note that this algorithm calculates a distance _iff_ d == a.
 */
template < bool _CaseSentitive >
static uint32_t LevenshteinDistanceImpl(const char * str1, uint32_t len1,
                                        const char * str2, uint32_t len2,
                                        uint32_t w, uint32_t s, uint32_t a, uint32_t d )
{
    ASSERT( 0 == len1 || nullptr != str1 );
    ASSERT( 0 == len2 || nullptr != str2 );

    if ( 0 == len1 || nullptr == str1 ) return ( a * len2 );
    if ( 0 == len2 || nullptr == str2 ) return ( d * len1 );

    typedef CharEqual< _CaseSentitive > equalto;

    uint32_t * row0 = ( uint32_t* )ALLOC( sizeof( uint32_t ) * ( len2 + 1 ) );
    uint32_t * row1 = ( uint32_t* )ALLOC( sizeof( uint32_t ) * ( len2 + 1 ) );
    uint32_t * row2 = ( uint32_t* )ALLOC( sizeof( uint32_t ) * ( len2 + 1 ) );

    for ( uint32_t j = 0 ; j <= len2 ; ++j )
    {
        row1[ 1 ] = j * a;
    }

    for ( uint32_t i = 0 ; i < len1 ; ++i )
    {
        row2[ 0 ] = ( i + 1 ) * d;

        for ( uint32_t j = 0 ; j < len2 ; ++j )
        {
            /* substitution */
            row2[ j + 1 ] = row1[ j ] + ( equalto()( str1[ i ], str2[ j ] ) ? 0 : s );
            /* swap */
            if ( i > 0 && j > 0 &&
                equalto()( str1[ i - 1 ], str2[ j ] ) &&
                equalto()( str1[ i ], str2[ j - 1 ] ) &&
                row2[ j + 1 ] > row0[ j - 1 ] + w )
            {
                row2[ j + 1 ] = row0[ j - 1 ] + w;
            }
            /* deletion */
            if ( row2[ j + 1 ] > row1[ j + 1 ] + d )
            {
                row2[ j + 1 ] = row1[ j + 1 ] + d;
            }
            /* insertion */
            if ( row2[ j + 1 ] > row2[ j ] + a )
            {
                row2[ j + 1 ] = row2[ j ] + a;
            }
        }

        auto dummy = row0;
        row0 = row1;
        row1 = row2;
        row2 = dummy;
    }

    const uint32_t result = row1[ len2 ];

    FREE( row0 );
    FREE( row1 );
    FREE( row2 );

    return ( result );
}

// Distance
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::Distance( const char * lhs, const char * rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d )
{
    const uint32_t lhsLen = ( nullptr == lhs ) ? 0 : ( uint32_t ) AString::StrLen( lhs );
    const uint32_t rhsLen = ( nullptr == rhs ) ? 0 : ( uint32_t ) AString::StrLen( rhs );
    return ::LevenshteinDistanceImpl< true >( lhs, lhsLen, rhs, rhsLen, w, s, a, d );
}

// Distance
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::Distance( const AString & lhs, const AString & rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d )
{
    return ::LevenshteinDistanceImpl< true >( lhs.Get(), lhs.GetLength(), rhs.Get(), rhs.GetLength(), w, s, a, d );
}

// DistanceI
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::DistanceI( const char * lhs, const char * rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d )
{
    const uint32_t lhsLen = ( nullptr == lhs ) ? 0 : ( uint32_t ) AString::StrLen( lhs );
    const uint32_t rhsLen = ( nullptr == rhs ) ? 0 : ( uint32_t ) AString::StrLen( rhs );
    return ::LevenshteinDistanceImpl< false >( lhs, lhsLen, rhs, rhsLen, w, s, a, d );
}

// DistanceI
//------------------------------------------------------------------------------
/*static*/ uint32_t LevenshteinDistance::DistanceI( const AString & lhs, const AString & rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d )
{
    return ::LevenshteinDistanceImpl< false >( lhs.Get(), lhs.GetLength(), rhs.Get(), rhs.GetLength(), w, s, a, d );
}

//------------------------------------------------------------------------------
