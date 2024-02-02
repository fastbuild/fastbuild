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

// ShellSort
//------------------------------------------------------------------------------
template < class T, class COMPARE >
void ShellSort( T * begin, T * end, const COMPARE & compare )
{
    // Ciura, Marcin (2001). "Best Increments for the Average Case of Shellsort".
    static const size_t gaps[] = { 510774, 227011, 100894, 44842, 19930, 8858, 3937, 1750, 701, 301, 132, 57, 23, 10, 4, 1 };

    const size_t numItems = (size_t)( end - begin );
    for ( const size_t increment : gaps )
    {
        if ( increment > numItems )
        {
            continue;
        }

        for ( size_t i = 0; i < numItems; i++ )
        {
            size_t j = i;
            T temp( Move( begin[ i ] ) );
            while ( ( j >= increment ) && ( compare( temp, begin[ j - increment ] ) ) )
            {
                begin[ j ] = Move( begin[ j - increment ] );
                j = j - increment;
            }
            begin[ j ] = Move( temp );
        }
    }
}

//------------------------------------------------------------------------------
