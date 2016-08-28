// Conversion.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Math
//------------------------------------------------------------------------------
namespace Math
{
    template <class T>
    static inline T RoundUp( T value, T alignment )
    {
        return ( value + alignment - 1) & ~( alignment - 1 );
    }
    template <class T>
    static inline T Max( T a, T b )
    {
        return ( a > b ) ? a : b;
    }
    template <class T>
    static inline T Min( T a, T b )
    {
        return ( a < b ) ? a : b;
    }
    template <class T>
    static inline T Clamp( T a, T b, T c )
    {
        return Min( Max( a, b ), c );
    }
    template <typename T>
    static inline bool IsPowerOf2( T value )
    {
        return ( ( ( value - 1 ) & value ) == 0 );
    }

};

//------------------------------------------------------------------------------
