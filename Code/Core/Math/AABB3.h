// AABB3
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_MATH_AABB3_H
#define CORE_MATH_AABB3_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Math/Vec3.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Ray3;

// AABB3
//------------------------------------------------------------------------------
class AABB3
{
public:
	inline explicit AABB3( const Vec3 & mi, const Vec3 & ma )
		: m_Min( mi ), m_Max( ma )
	{}
	inline AABB3() {}
	inline ~AABB3() {}

	bool Intersect( const Ray3 & ray, float & dist ) const;
	
private:
	Vec3 m_Min;
	Vec3 m_Max;
};

//------------------------------------------------------------------------------
#endif // CORE_MATH_AABB3_H
