// AABB3
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "AABB3.h"
#include "Core/Math/Ray3.h"

// Intersect (Ray3)
//------------------------------------------------------------------------------
bool AABB3::Intersect( const Ray3 & ray, float & dist ) const
{
    const float ox = ray.GetOrigin().x;
    const float oy = ray.GetOrigin().y;
    const float oz = ray.GetOrigin().z;
    const float dx = ray.GetDir().x;
    const float dy = ray.GetDir().y;
    const float dz = ray.GetDir().z;

    float tx_min, ty_min, tz_min;
    float tx_max, ty_max, tz_max;

    // x
    float a = 1.f/dx;
    if ( a >= 0 )
    {
        tx_min = ( m_Min.x-ox ) * a;
        tx_max = ( m_Max.x-ox ) * a;
    }
    else
    {
        tx_min = ( m_Max.x-ox ) * a;
        tx_max = ( m_Min.x-ox ) * a;
    }

    // y
    float b = 1.f/dy;
    if ( b >= 0 )
    {
        ty_min = ( m_Min.y-oy ) * b;
        ty_max = ( m_Max.y-oy ) * b;
    }
    else
    {
        ty_min = ( m_Max.y-oy ) * b;
        ty_max = ( m_Min.y-oy ) * b;
    }

    // z
    float c = 1.f/dz;
    if ( c >= 0 )
    {
        tz_min = ( m_Min.z-oz ) * c;
        tz_max = ( m_Max.z-oz ) * c;
    }
    else
    {
        tz_min = ( m_Max.z-oz ) * c;
        tz_max = ( m_Min.z-oz ) * c;
    }

    float t0, t1;

    // find largest entering t-value
    t0 = tx_min > ty_min ? tx_min : ty_min;
    t0 = tz_min > t0 ? tz_min : t0;

    // find smallest exiting t-value
    t1 = tx_max < ty_max ? tx_max : ty_max;
    t1 = tz_max < t1 ? tz_max : t1;

    if ( t0 < t1 && t1 > 0.001f ) // EPSILON
    {
        dist = t0;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
