// Mat44.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/PrecompiledHeader.h"
#include "Mat44.h"
#include "Core/Math/Vec3.h"

// system
#include <memory.h>

// Static Data
//------------------------------------------------------------------------------
/*static*/ float Mat44::s_Identity[] = { 1.0f, 0.0f, 0.0f, 0.0f,
                                         0.0f, 1.0f, 0.0f, 0.0f,
                                         0.0f, 0.0f, 1.0f, 0.0f,
                                         0.0f, 0.0f, 0.0f, 1.0f };

// MakeIdentity()
//------------------------------------------------------------------------------
void Mat44::MakeIdentity()
{
    col0 = Vec4( 1.0f, 0.0f, 0.0f, 0.0f );
    col1 = Vec4( 0.0f, 1.0f, 0.0f, 0.0f );
    col2 = Vec4( 0.0f, 0.0f, 1.0f, 0.0f );
    col3 = Vec4( 0.0f, 0.0f, 0.0f, 1.0f );
}

// MakeLookAt
//------------------------------------------------------------------------------
void Mat44::MakeLookAt( const Vec3 & pos, const Vec3 & lookAt, const Vec3 & upVector )
{
    // generate the forward vector
    Vec3 forward( pos - lookAt );
    forward.Normalise();

    Vec3 right = upVector.Cross( forward );
    right.Normalise();

    Vec3 up = forward.Cross( right );
    up.Normalise();

    col0 = Vec4( right.x, up.x, forward.x, 0.0f );
    col1 = Vec4( right.y, up.y, forward.y, 0.0f ),
    col2 = Vec4( right.z, up.z, forward.z, 0.0f ),
    col3 = Vec4( -right.Dot( pos ), -up.Dot( pos ), -forward.Dot( pos ), 1.0f );
}

// SetTranslation()
//------------------------------------------------------------------------------
void Mat44::SetTranslation( const Vec3 & tran )
{
    col3.x = tran.x;
    col3.y = tran.y;
    col3.z = tran.z;
}

// operator * (Mat44)
//------------------------------------------------------------------------------
Mat44 Mat44::operator *(const Mat44 & other ) const
{
    return Mat44( (*this) * other.col0,
                  (*this) * other.col1,
                  (*this) * other.col2,
                  (*this) * other.col3 );
}

// MakeRotationX()
//------------------------------------------------------------------------------
void Mat44::MakeRotationX( float angleRadians )
{
    float cosa = Cos(angleRadians);
    float sina = Sin(angleRadians);

    MakeIdentity();

    col1.y = cosa;
    col2.z = cosa;
    col1.z = -sina;
    col2.y = sina;
}

// MakeRotationY()
//------------------------------------------------------------------------------
void Mat44::MakeRotationY( float angleRadians )
{
    float cosa = Cos(angleRadians);
    float sina = Sin(angleRadians);

    MakeIdentity();

    col0.x = cosa;
    col2.z = cosa;
    col0.z = sina;
    col2.x = -sina;
}

// MakeRotationZ()
//------------------------------------------------------------------------------
void Mat44::MakeRotationZ( float angleRadians )
{
    float cosa = Cos(angleRadians);
    float sina = Sin(angleRadians);

    MakeIdentity();

    col0.x = cosa;
    col1.y = cosa;
    col0.y = -sina;
    col1.x = sina;
}

// MakeScale
//------------------------------------------------------------------------------
void Mat44::MakeScale( float scale )
{
    col0 = Vec4( scale, 0.0f, 0.0f, 0.0f );
    col1 = Vec4( 0.0f, scale, 0.0f, 0.0f );
    col2 = Vec4( 0.0f, 0.0f, scale, 0.0f );
    col3 = Vec4( 0.0f, 0.0f, 0.0f, 1.0f );
}

// MakeProjection
//------------------------------------------------------------------------------
void Mat44::MakeProjection( float yFOV, float aspect, float zNear, float zFar )
{
    const float ymax( zNear * Tan( yFOV * 0.5f ) );
    const float xmax( ymax * aspect );
    const float x( zNear / xmax );
    const float y( zNear / ymax );
    const float c( -( zFar + zNear ) / ( zFar - zNear ) );
    const float d( -( 2.0f * zFar * zNear ) / ( zFar - zNear ) );

    col0 = Vec4(    x, 0.0f, 0.0f,  0.0f );
    col1 = Vec4( 0.0f,    y, 0.0f,  0.0f );
    col2 = Vec4( 0.0f, 0.0f,    c, -1.0f );
    col3 = Vec4( 0.0f, 0.0f,    d,  0.0f );
}

//------------------------------------------------------------------------------
