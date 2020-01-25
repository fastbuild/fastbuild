// PropertyType.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
template< class T > class Array;

// PropertyType
//------------------------------------------------------------------------------
enum PropertyType : uint8_t
{
    PT_NONE         = 0,
    PT_FLOAT        = 1,
    PT_UINT8        = 2,
    PT_UINT16       = 3,
    PT_UINT32       = 4,
    PT_UINT64       = 5,
    PT_INT8         = 6,
    PT_INT16        = 7,
    PT_INT32        = 8,
    PT_INT64        = 9,
    PT_BOOL         = 10,
    PT_ASTRING      = 11,
    PT_STRUCT       = 16,
};

inline PropertyType GetPropertyType( const float * )    { return PT_FLOAT; }
inline PropertyType GetPropertyType( const uint8_t * )  { return PT_UINT8; }
inline PropertyType GetPropertyType( const uint16_t * ) { return PT_UINT16; }
inline PropertyType GetPropertyType( const uint32_t * ) { return PT_UINT32; }
inline PropertyType GetPropertyType( const uint64_t * ) { return PT_UINT64; }
inline PropertyType GetPropertyType( const int8_t * )   { return PT_INT8; }
inline PropertyType GetPropertyType( const int16_t * )  { return PT_INT16; }
inline PropertyType GetPropertyType( const int32_t * )  { return PT_INT32; }
inline PropertyType GetPropertyType( const int64_t * )  { return PT_INT64; }
inline PropertyType GetPropertyType( const bool * )     { return PT_BOOL; }
inline PropertyType GetPropertyType( const AString * )  { return PT_ASTRING; }
template < class T >
inline PropertyType GetPropertyArrayType( const Array< T > * )
{
    return GetPropertyType( static_cast< T * >( nullptr ) );
}

//------------------------------------------------------------------------------
