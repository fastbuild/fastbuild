// Object.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ReflectionInfo;

// Object
//------------------------------------------------------------------------------
class Object
{
public:
    inline explicit Object() = default;
    inline virtual ~Object() = default;

    virtual const ReflectionInfo * GetReflectionInfoV() const = 0;

private:
    template< class T, class U >
    friend T * DynamicCast( U * object );
    static bool CanDynamicCast( const ReflectionInfo * dst, const ReflectionInfo * src );
};
void Object_ReflectionInfo_Bind();

// DynamicCast
//------------------------------------------------------------------------------
template < class T, class U >
T * DynamicCast( U * object )
{
    if ( object )
    {
        if ( Object::CanDynamicCast( T::GetReflectionInfoS(), object->GetReflectionInfoV() ) )
        {
            return (T *)object;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
