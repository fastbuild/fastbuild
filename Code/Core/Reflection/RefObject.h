// RefObject.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ReflectionInfo;

// RefObject
//------------------------------------------------------------------------------
class RefObject
{
public:
    inline explicit RefObject() : m_ReferenceCount( 0 ) {}
    inline virtual ~RefObject() = default;

    virtual const ReflectionInfo * GetReflectionInfoV() const = 0;

    // TODO: This should be private
    inline void IncRef()
    {
        ++m_ReferenceCount;
    }
    // TODO: This should be private
    inline void DecRef()
    {
        if ( --m_ReferenceCount == 0 )
        {
            Destroy();
        }
    }

private:
    template< class T, class U >
    friend T * DynamicCast( U * object );
    static bool CanDynamicCast( const ReflectionInfo * dst, const ReflectionInfo * src );

    inline virtual void Destroy() { delete this; }

    uint32_t m_ReferenceCount;
};
void RefObject_ReflectionInfo_Bind();

// DynamicCast
//------------------------------------------------------------------------------
template < class T, class U >
T * DynamicCast( U * object )
{
    if ( object )
    {
        if ( RefObject::CanDynamicCast( T::GetReflectionInfoS(), object->GetReflectionInfoV() ) )
        {
            return (T *)object;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
