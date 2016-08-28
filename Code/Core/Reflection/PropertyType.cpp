// PropertyType.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"
#include "PropertyType.h"

// Core
#include "Core/Strings/AString.h"

// GetPropertyTypeFromString
//------------------------------------------------------------------------------
PropertyType GetPropertyTypeFromString( const AString & propertyType )
{
    if ( propertyType == "float" )
    {
        return PT_FLOAT;
    }

    if ( propertyType == "u8" )
    {
        return PT_UINT8;
    }

    if ( propertyType == "u16" )
    {
        return PT_UINT16;
    }

    if ( propertyType == "u32" )
    {
        return PT_UINT32;
    }

    if ( propertyType == "u64" )
    {
        return PT_UINT64;
    }

    if ( propertyType == "i8" )
    {
        return PT_INT8;
    }

    if ( propertyType == "i16" )
    {
        return PT_INT16;
    }

    if ( propertyType == "i32" )
    {
        return PT_INT32;
    }

    if ( propertyType == "i64" )
    {
        return PT_INT64;
    }

    if ( propertyType == "bool" )
    {
        return PT_BOOL;
    }

    if ( propertyType == "aStr" )
    {
        return PT_ASTRING;
    }

    if ( propertyType == "vec2" )
    {
        return PT_VEC2;
    }

    if ( propertyType == "vec3" )
    {
        return PT_VEC3;
    }

    if ( propertyType == "vec4" )
    {
        return PT_VEC4;
    }

    if ( propertyType == "mat44" )
    {
        return PT_MAT44;
    }

    return PT_NONE;
}

//------------------------------------------------------------------------------
