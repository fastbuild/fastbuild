// PropertyType.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class ReflectionInfo;
class Struct;
template <class T> class Array;

// PropertyType
//------------------------------------------------------------------------------
enum PropertyType : uint8_t
{
    PT_NONE = 0,
    PT_FLOAT = 1,
    PT_UINT8 = 2,
    PT_UINT16 = 3,
    PT_UINT32 = 4,
    PT_UINT64 = 5,
    PT_INT8 = 6,
    PT_INT16 = 7,
    PT_INT32 = 8,
    PT_INT64 = 9,
    PT_BOOL = 10,
    PT_ASTRING = 11,
    PT_STRUCT = 16,

    PT_CUSTOM_1 = 128,
};

[[nodiscard]] inline constexpr PropertyType GetPropertyType( const float * )
{
    return PT_FLOAT;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const uint8_t * )
{
    return PT_UINT8;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const uint16_t * )
{
    return PT_UINT16;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const uint32_t * )
{
    return PT_UINT32;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const uint64_t * )
{
    return PT_UINT64;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const int8_t * )
{
    return PT_INT8;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const int16_t * )
{
    return PT_INT16;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const int32_t * )
{
    return PT_INT32;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const int64_t * )
{
    return PT_INT64;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const bool * )
{
    return PT_BOOL;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const AString * )
{
    return PT_ASTRING;
}
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const Struct * )
{
    return PT_STRUCT;
}
template <class T>
[[nodiscard]] inline constexpr PropertyType GetPropertyType( const Array<T> * )
{
    return GetPropertyType( static_cast<T *>( nullptr ) );
}

//------------------------------------------------------------------------------
template <class T>
[[nodiscard]] inline constexpr bool IsArrayProperty( const Array<T> * )
{
    return true;
}

//------------------------------------------------------------------------------
template <class T>
[[nodiscard]] inline constexpr bool IsArrayProperty( const T * )
{
    return false;
}

//------------------------------------------------------------------------------
template <class T>
[[nodiscard]] inline constexpr bool IsStruct( const T * single )
{
    return ( GetPropertyType( single ) == PropertyType::PT_STRUCT );
}

//------------------------------------------------------------------------------
[[nodiscard]] inline constexpr bool IsStruct( const Struct * /*single*/ )
{
    return true;
}

//------------------------------------------------------------------------------
template <class T>
[[nodiscard]] inline constexpr bool IsStruct( const Array<T> * /*array*/ )
{
    return IsStruct( (T *)nullptr );
}

//------------------------------------------------------------------------------
template <class T>
[[nodiscard]] inline constexpr const ReflectionInfo * GetStructType( const T * /*single*/ )
{
    if constexpr ( IsStruct( (T *)nullptr ) )
    {
        return T::GetReflectionInfoS();
    }
    else
    {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
template <class T>
[[nodiscard]] inline constexpr const ReflectionInfo * GetStructType( const Array<T> * /*array*/ )
{
    if constexpr ( IsStruct( (T *)nullptr ) )
    {
        return T::GetReflectionInfoS();
    }
    else
    {
        return nullptr;
    }
}

//------------------------------------------------------------------------------
