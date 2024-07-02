// MemInfo.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// SystemMemInfo
//------------------------------------------------------------------------------
class SystemMemInfo
{
public:
    // Physical memory
    uint32_t    mTotalPhysMiB = 0; // Usable by OS (<= physically installed)
    uint32_t    mAvailPhysMiB = 0; // Free
};

// MemInfo
//------------------------------------------------------------------------------
class MemInfo
{
public:
    // Obtain information about the system
    static void GetSystemInfo( SystemMemInfo & outInfo );

protected:
    // Internal helpers
    static uint32_t    ConvertBytesToMiB( uint64_t bytes );
};

//------------------------------------------------------------------------------
