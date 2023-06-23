// Sort.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// AscendingCompare
//------------------------------------------------------------------------------
class AscendingCompare
{
public:
    template < class T >
    inline bool operator () ( const T & a, const T & b ) const
    {
        return ( a < b );
    }
};

// AscendingCompareDeref
//------------------------------------------------------------------------------
class AscendingCompareDeref
{
public:
    template < class T >
    inline bool operator () ( const T & a, const T & b ) const
    {
        return ( ( *a ) < ( *b ) );
    }
};

template < class T > 
static inline void   SortSwap( T & a, T & b )      { T tmp( a ); a = b; b = tmp; }
static inline size_t SortMin( size_t a, size_t b ) { return a < b ? a : b; }

// QuickSort
//------------------------------------------------------------------------------
template < class T, class COMPARE >
void QuickSort( T * begin, T * end, const COMPARE & less )
{
    struct Stack { T *arr, *end; } stack[ 64 ], range = { begin, end };
    int depth = 0;

    for (;;)
    {
        // Sort small runs with O(n^2) insertion sort, large with O(log n) quick sort.
        size_t count = size_t( range.end - range.arr );
        if ( count <= 16 || depth >= 63 )
        {
            for ( size_t i = 1; i < count; i++ )
            {
                T tmp( range.arr[ i ] );
                size_t j = i;
                while ( j > 0 && less( tmp, range.arr[ j - 1 ] ) )
                {
                    range.arr[ j ] = range.arr[ j - 1 ];
                    j--;
                }
                range.arr[ j ] = tmp;
            }

            // Pop new range off stack.
            if ( depth <= 0 )
                break;
            range = stack[ --depth ];
        } else {
            // Pick median-of-3 as pivot.
            T *a = range.arr, *b = range.arr + ( count >> 1 ), *c = range.end - 1;
            if ( less( *c, *a ) ) SortSwap( *a, *c );
            if ( less( *b, *c ) ) SortSwap( *c, *b );
            if ( less( *c, *a ) ) SortSwap( *a, *c );
            T pivot(*c);

            // Partition into (eq, lt, gt, eq). (Bentley/McIlroy, "Engineering a Sort Function", 1993)
            size_t i = 0, j = count - 1, ieq = 0, jeq = count - 1;
            for (;;)
            {
                while ( i <= j && !less( pivot, range.arr[ i ] ) )
                {
                    if ( !less( range.arr[ i ], pivot ) )
                        SortSwap( range.arr[ ieq++ ], range.arr[ i ] );
                    i++;
                }
                while ( j >= i && !less( range.arr[ j ], pivot ) )
                {
                    if (!less( pivot, range.arr[ j ] ) )
                        SortSwap( range.arr[ jeq-- ], range.arr[ j ] );
                    j--;
                }
                if (i > j)
                    break;
                SortSwap( range.arr[ i++ ], range.arr[ j-- ] );
            }

            // Move elements equal to the pivot back into the middle.
            size_t s = SortMin( ieq, i - ieq );
            for (size_t l = 0, h = i - s; s; s--)
                SortSwap( range.arr[ l++ ], range.arr[ h++ ] );
            s = SortMin( jeq - j, count - 1 - jeq );
            for (size_t l = i, h = count - s; s; s--)
                SortSwap( range.arr[ l++ ], range.arr[ h++ ] );

            // Save one half for later and recurse into the other.
            stack[ depth++ ] = { range.arr + count - ( jeq - j ), range.end };
            range = { range.arr, range.arr + ( i - ieq ) };
        }
    }
}

//------------------------------------------------------------------------------
