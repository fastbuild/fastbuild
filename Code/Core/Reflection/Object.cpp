// Object.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Object.h"
#include "Core/Reflection/ReflectionInfo.h"

// Reflection
//------------------------------------------------------------------------------
// Object_ReflectionInfo
//------------------------------------------------------------------------------
class Object_ReflectionInfo : public ReflectionInfo
{
public:
    explicit Object_ReflectionInfo() { SetTypeName( "Object" ); m_IsAbstract = true; }
    virtual ~Object_ReflectionInfo() override = default;
};
Object_ReflectionInfo g_Object_ReflectionInfo;

// DynamicCastHelper
//------------------------------------------------------------------------------
/*static*/ bool Object::CanDynamicCast( const ReflectionInfo * dst, const ReflectionInfo * src )
{
    while ( src )
    {
        if ( src == dst )
        {
            return true;
        }
        src = src->GetSuperClass();
    }
    return false;
}

//------------------------------------------------------------------------------
