// Vec4.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_MATH_VEC4_H
#define CORE_MATH_VEC4_H

// Includes
//------------------------------------------------------------------------------

// Vec4
//------------------------------------------------------------------------------
class Vec4
{
public:
	float	x,y,z,w;
	
	inline explicit Vec4() {}
	inline explicit Vec4( float x1, float y1, float z1, float w1 ) { x=x1; y=y1; z=z1; w=w1; }
	inline ~Vec4() {}

	inline Vec4 operator *( float f ) const;
	inline Vec4 operator +( const Vec4 & other ) const;
	inline Vec4 operator -( const Vec4 & other ) const;
	inline Vec4 & operator = ( const Vec4 & other );

	inline bool operator ==( const Vec4 & other ) const;
};

// operator * (float)
//------------------------------------------------------------------------------
/*inline*/ Vec4 Vec4::operator *( float f ) const
{
	return Vec4( x * f, y * f, z * f, w * f );
}

// operator +
//------------------------------------------------------------------------------
/*inline*/ Vec4 Vec4::operator +( const Vec4 & other ) const
{
	return Vec4( x + other.x, y + other.y, z + other.z, w + other.w );
}

// operator -
//------------------------------------------------------------------------------
/*inline*/ Vec4 Vec4::operator -( const Vec4 & other ) const
{
	return Vec4( x - other.x, y - other.y, z - other.z, w - other.w );
}

// operator =
//------------------------------------------------------------------------------
/*inline*/ Vec4 & Vec4::operator = ( const Vec4 & other )
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
	return *this;
}

// operator ==
//------------------------------------------------------------------------------
/*inline*/ bool Vec4::operator ==( const Vec4 & vec ) const
{
	return ( ( x == vec.x ) && ( y == vec.y ) && ( z == vec.z ) && ( w == vec.w ) );
}

//------------------------------------------------------------------------------
#endif // CORE_MATH_VEC4_H
