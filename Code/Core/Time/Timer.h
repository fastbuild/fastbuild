// Timer.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Timer
//------------------------------------------------------------------------------
class Timer
{
public:
    Timer() { Start(); }
    ~Timer() = default;

    void Start() { m_StartTime = GetNow(); }
    void Start( float time ) { m_StartTime = GetNow() - (int64_t)( (double)GetFrequency() * (double)time ); }

    float GetElapsed() const
    {
        const int64_t now = GetNow();
        return ( (float)( now - m_StartTime ) * GetFrequencyInvFloat() );
    }

    float GetElapsedMS() const
    {
        const int64_t now = GetNow();
        return ( (float)( now - m_StartTime ) * GetFrequencyInvFloatMS() );
    }

    static int64_t GetNow();
    static int64_t GetFrequency() { return s_Frequency; }
    static float GetFrequencyInvFloat() { return s_FrequencyInvFloat; }
    static float GetFrequencyInvFloatMS() { return s_FrequencyInvFloatMS; }

private:
    int64_t m_StartTime;

    // frequency
    friend class GlobalTimerFrequencyInitializer;
    static int64_t s_Frequency;
    static float s_FrequencyInvFloat;
    static float s_FrequencyInvFloatMS;
};

//------------------------------------------------------------------------------
