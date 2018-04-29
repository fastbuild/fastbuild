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
    static uint32_t Distance( const char * lhs, const char * rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d );
    static uint32_t Distance( const AString & lhs, const AString & rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d );

    static uint32_t DistanceI( const char * lhs, const char * rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d );
    static uint32_t DistanceI( const AString & lhs, const AString & rhs, uint32_t w, uint32_t s, uint32_t a, uint32_t d );

    static uint32_t Distance( const char * lhs, const char * rhs ) { return Distance( lhs, rhs, 1, 1, 1, 1 ); }
    static uint32_t Distance( const AString & lhs, const AString & rhs ) { return Distance( lhs, rhs, 1, 1, 1, 1 ); }

    static uint32_t DistanceI( const char * lhs, const char * rhs ) { return Distance( lhs, rhs, 1, 1, 1, 1 ); }
    static uint32_t DistanceI( const AString & lhs, const AString & rhs ) { return Distance( lhs, rhs, 1, 1, 1, 1 ); }
};

//------------------------------------------------------------------------------
