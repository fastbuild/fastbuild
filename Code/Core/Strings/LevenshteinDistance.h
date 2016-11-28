// LevenshteinDistance.h
//------------------------------------------------------------------------------
#pragma once

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
