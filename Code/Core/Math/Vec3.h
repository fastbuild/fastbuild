// Vec3.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Math/Trig.h"

// Vec3
//------------------------------------------------------------------------------
class Vec3
{
public:
    float   x,y,z;

    inline explicit Vec3() = default;
    inline explicit Vec3(float x1, float y1, float z1) { x=x1; y=y1; z=z1; }
    inline ~Vec3() = default;

    // basic operators
    inline void  operator = (const Vec3 &vec);
    inline Vec3  operator + (const Vec3 &vec) const;
    inline Vec3  operator - (const Vec3 &vec) const;
    inline void  operator +=(const Vec3 &vec);
    inline void  operator -=(const Vec3 &vec);
    inline bool  operator ==( const Vec3 & vec ) const;

    inline Vec3 operator - () const;
    inline Vec3 operator * (const float f) const;
    inline Vec3 operator / (const float f) const;
    inline void operator *=(const float f);

    inline Vec3 Cross( const Vec3 & other ) const;
    inline float Dot( const Vec3 & other ) const;

    // convenience functions
    inline void  Normalise();
    inline float GetLength() const;
    inline float GetLengthSquared() const;

    // constants
    inline static const Vec3 & GetZero() { return *( (const Vec3 *)s_Zero ); }
    inline static const Vec3 & GetOne()  { return *( (const Vec3 *)s_One); }

private:
    static const float s_Zero[ 3 ];
    static const float s_One[ 3 ];
};

// operator =
//------------------------------------------------------------------------------
void Vec3::operator = (const Vec3 &vec)
{
    x = vec.x;
    y = vec.y;
    z = vec.z;
}

// operator +
//------------------------------------------------------------------------------
Vec3 Vec3::operator + (const Vec3 &vec) const
{
    Vec3 result;
    result.x = x + vec.x;
    result.y = y + vec.y;
    result.z = z + vec.z;
    return result;
}

// operator -
//------------------------------------------------------------------------------
Vec3 Vec3::operator - (const Vec3 &vec) const
{
    Vec3 result;
    result.x = x - vec.x;
    result.y = y - vec.y;
    result.z = z - vec.z;
    return result;
}

// operator +=
//------------------------------------------------------------------------------
void Vec3::operator +=(const Vec3 &vec)
{
    x += vec.x;
    y += vec.y;
    z += vec.z;
}

// operator -=
//------------------------------------------------------------------------------
void Vec3::operator -=(const Vec3 &vec)
{
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;
}

// operator ==
//------------------------------------------------------------------------------
bool Vec3::operator ==( const Vec3 & vec ) const
{
    return ( ( x == vec.x ) && ( y == vec.y ) && ( z == vec.z ));
}

// operator -
//------------------------------------------------------------------------------
Vec3 Vec3::operator - () const
{
    return Vec3( -x, -y, -z );
}

// operator * (float)
//------------------------------------------------------------------------------
Vec3 Vec3::operator *(const float f) const
{
    return Vec3( x * f, y * f, z * f );
}

// operator / (float)
//------------------------------------------------------------------------------
Vec3 Vec3::operator / (const float f) const
{
    return Vec3( x / f, y / f, z / f );
}

// operator *= (float)
//------------------------------------------------------------------------------
void Vec3::operator *=(const float f)
{
    x *= f;
    y *= f;
    z *= f;
}

// Cross
//------------------------------------------------------------------------------
Vec3 Vec3::Cross( const Vec3 & other ) const
{
    Vec3 result;
    result.x = (y*other.z)-(other.y*z);
    result.y = -(x*other.z)+(other.x*z);
    result.z = (x*other.y)-(y*other.x);
    return result;
}

// Dot
//------------------------------------------------------------------------------
float Vec3::Dot( const Vec3 & other ) const
{
    return ( x * other.x ) + ( y * other.y ) + ( z * other.z );
}

// Normalise - make vector unit length
//------------------------------------------------------------------------------
void Vec3::Normalise()
{
    float length = GetLength();
    x /= length;
    y /= length;
    z /= length;
}

// GetLength - return length of vector
//------------------------------------------------------------------------------
float Vec3::GetLength() const
{
    return Sqrt((x*x)+(y*y)+(z*z));
}

// GetLengthSquared - return squared length of vector
//------------------------------------------------------------------------------
float Vec3::GetLengthSquared() const
{
    return ((x*x)+(y*y)+(z*z));
}

//------------------------------------------------------------------------------
