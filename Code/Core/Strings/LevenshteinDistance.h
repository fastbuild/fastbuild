// LevenshteinDistance.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_STRINGS_LEVENSHTEINDISTANCE_H
#define CORE_STRINGS_LEVENSHTEINDISTANCE_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declare
//------------------------------------------------------------------------------
class AString;

// LevenshteinDistance
//------------------------------------------------------------------------------
class LevenshteinDistance
{
public:
    static uint32_t Distance( const char * lhs, const char * rhs );
    static uint32_t Distance( const AString & lhs, const AString & rhs );

    static uint32_t DistanceI( const char * lhs, const char * rhs );
    static uint32_t DistanceI( const AString & lhs, const AString & rhs );
};

//------------------------------------------------------------------------------
#endif // CORE_STRINGS_LEVENSHTEINDISTANCE_H
