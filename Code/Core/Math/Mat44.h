// Mat44.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Math/Vec3.h"
#include "Core/Math/Vec4.h"

// Mat44
//------------------------------------------------------------------------------
class Mat44
{
public:
    Vec4 col0;
    Vec4 col1;
    Vec4 col2;
    Vec4 col3;

    inline explicit Mat44() = default;
    inline explicit Mat44( const Vec4 & c0, const Vec4 & c1, const Vec4 & c2, const Vec4 & c3 )
                    : col0( c0 ), col1( c1 ), col2( c2 ), col3( c3 ) {}
    inline         ~Mat44() = default;

    // modify the array
    void                SetTranslation( const Vec3 &tran );

    // multiplications
    Mat44 operator *( const Mat44 & other ) const;
    inline Vec4 operator * ( const Vec4 & v ) const;

    inline bool operator ==( const Mat44 & other ) const;

    // access x/y/z component axis and translation
    //inline const Vec3 & GetXAxis() const      { return (const Vec3 &)col0; }
    //inline const Vec3 & GetYAxis() const      { return (const Vec3 &)col1; }
    //inline const Vec3 & GetZAxis() const      { return (const Vec3 &)col2; }
    //inline const Vec3 & GetTranslation() const    { return (const Vec3 &)col3; }

    void                MakeIdentity();
    void                MakeLookAt( const Vec3 & pos, const Vec3 & lookAt, const Vec3 & upVector );
    void                MakeRotationX( float angleRadians );
    void                MakeRotationY( float angleRadians );
    void                MakeRotationZ( float angleRadians );
    void                MakeScale( float scale );
    void                MakeProjection( float yFOV, float aspect, float zNear, float zFar );

    // constants
    inline static const Mat44 & GetIdentity() { return *( ( const Mat44 *)s_Identity ); }

private:
    static float s_Identity[ 16 ];
};

// operator * ( Vec4 )
//------------------------------------------------------------------------------
/*inline*/ Vec4 Mat44::operator * ( const Vec4 & v ) const
{
    return col0 * v.x + col1 * v.y + col2 * v.z + col3 * v.w;
}

// operator ==
//------------------------------------------------------------------------------
/*inline*/ bool Mat44::operator ==( const Mat44 & other ) const
{
    return ( ( col0 == other.col0 ) &&
             ( col1 == other.col1 ) &&
             ( col2 == other.col2 ) &&
             ( col3 == other.col3 ) );
}

//------------------------------------------------------------------------------
