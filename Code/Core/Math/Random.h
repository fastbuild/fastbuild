// Random
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Random
//------------------------------------------------------------------------------
class Random
{
public:
    static const uint32_t CORE_RAND_MAX = 32767;

    // seed with the current time
    Random();

    // seed with a specific value
    explicit inline Random( uint32_t seed ) : m_Seed( seed ) {}

    // random number from 0 to RAND_MAX
    uint32_t GetRand();

    // random float from 0.0f to 1.0f
    float GetRandFloat()
    {
        return ( (float)GetRand() ) / ( (float)CORE_RAND_MAX );
    }

    // random index from 0 to size-1
    uint32_t GetRandIndex( uint32_t size )
    {
        return ( (uint32_t)( (float)size * ( (float)GetRand() / (float)( CORE_RAND_MAX + 1 ) ) ) );
    }

    // access the seed value
    inline void     SetSeed( uint32_t seed ) { m_Seed = seed; }
    inline uint32_t GetSeed() const { return m_Seed; }

private:
    uint32_t m_Seed;
};

//------------------------------------------------------------------------------
