// Vec2.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Math/Trig.h"

// Vec2
//------------------------------------------------------------------------------
class Vec2
{
public:
    float   x,y;

    inline explicit Vec2() = default;
    inline explicit Vec2( float x1, float y1 ) { x=x1; y=y1; }
    inline ~Vec2() = default;

    // basic operators
    inline Vec2 operator + ( const Vec2 & vec ) const;
    inline Vec2 operator - ( const Vec2 & vec ) const;
    inline void operator +=( const Vec2 & vec );
    inline void operator -=( const Vec2 & vec );
    inline bool operator ==( const Vec2 & vec ) const;
    inline bool operator <=( const Vec2 & vec ) const;
    inline bool operator >=( const Vec2 & vec ) const;
    inline void operator /=( float val );
    inline void operator *=( float val );
    inline Vec2 operator * ( float val ) const;
    inline Vec2 operator - () const;

    // advanced operations
    inline void  Rotate90Degrees();
    inline float Dot( const Vec2 & vec ) const;

    // convenience functions
    inline void  Normalise();
    inline float GetLength() const;
    inline float GetLengthSquared() const;
};

// operator +
//------------------------------------------------------------------------------
Vec2 Vec2::operator + ( const Vec2 & vec ) const
{
    Vec2 result;
    result.x = x + vec.x;
    result.y = y + vec.y;
    return result;
}

// operator -
//------------------------------------------------------------------------------
Vec2 Vec2::operator - ( const Vec2 & vec ) const
{
    Vec2 result;
    result.x = x - vec.x;
    result.y = y - vec.y;
    return result;
}

// operator +=
//------------------------------------------------------------------------------
void Vec2::operator +=( const Vec2 & vec )
{
    x += vec.x;
    y += vec.y;
}

// operator -=
//------------------------------------------------------------------------------
void Vec2::operator -=( const Vec2 & vec )
{
    x -= vec.x;
    y -= vec.y;
}

// operator ==
//------------------------------------------------------------------------------
bool Vec2::operator ==( const Vec2 & vec ) const
{
    return ( ( x == vec.x ) && ( y == vec.y ) );
}

// operator >=
//------------------------------------------------------------------------------
bool Vec2::operator >=( const Vec2 & vec ) const
{
    if ((x >= vec.x) && (y >= vec.y))
        return true;
    return false;
}

// operator <=
//------------------------------------------------------------------------------
bool Vec2::operator <=( const Vec2 & vec ) const
{
    if ((x <= vec.x) && (y <= vec.y))
        return true;
    return false;
}


// Normalise - make vector unit length
//------------------------------------------------------------------------------
void Vec2::Normalise()
{
    float length = GetLength();
    x /= length;
    y /= length;
}

// GetLength - return length of vector
//------------------------------------------------------------------------------
float Vec2::GetLength() const
{
    return Sqrt((x*x)+(y*y));
}

// GetLengthSquared - return squared length of vector
//------------------------------------------------------------------------------
float Vec2::GetLengthSquared() const
{
    return ((x*x)+(y*y));
}

// Rotate()
//------------------------------------------------------------------------------
void Vec2::Rotate90Degrees()
{
    float origX = x;
    x = -y;
    y = origX;
}

// Dot()
//------------------------------------------------------------------------------
float Vec2::Dot( const Vec2 & vec ) const
{
    return (x*vec.x) + (y*vec.y);
}

// operator /=
//------------------------------------------------------------------------------
void Vec2::operator /= ( float val )
{
    x /= val;
    y /= val;
}

// operator *=
//------------------------------------------------------------------------------
void Vec2::operator *= ( float val )
{
    x *= val;
    y *= val;
}

// operator *
//------------------------------------------------------------------------------
Vec2 Vec2::operator * ( float val ) const
{
    return Vec2( x * val, y * val );
}

// operator -
//------------------------------------------------------------------------------
Vec2 Vec2::operator - () const
{
    return Vec2( -x, -y );
}

//------------------------------------------------------------------------------
