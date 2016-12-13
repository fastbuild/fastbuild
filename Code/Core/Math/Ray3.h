// Ray3
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Math/Vec3.h"

// Ray3
//------------------------------------------------------------------------------
class Ray3
{
public:
    inline explicit Ray3( const Vec3 & origin, const Vec3 & dir )
        : m_Origin( origin )
        , m_Direction( dir )
    {}
    inline Ray3() = default;
    inline ~Ray3() = default;

    inline const Vec3 & GetOrigin() const { return m_Origin; }
    inline const Vec3 & GetDir() const { return m_Direction; }

private:
    Vec3 m_Origin;
    Vec3 m_Direction;
};

//------------------------------------------------------------------------------
