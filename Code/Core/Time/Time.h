// Time.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

// Time
//------------------------------------------------------------------------------
class Time
{
public:
    static uint64_t GetCurrentFileTime();
    static void FormatTime( const float timeInSeconds , const bool outputFractionalDigits, AString & buffer  );
};

//------------------------------------------------------------------------------
