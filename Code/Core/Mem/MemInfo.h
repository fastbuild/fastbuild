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
    uint32_t m_TotalPhysMiB = 0; // Usable by OS (<= physically installed)
    uint32_t m_AvailPhysMiB = 0; // Free
};

// MemInfo
//------------------------------------------------------------------------------
class MemInfo
{
public:
    // Obtain information about the system
    static void GetSystemInfo( SystemMemInfo & outInfo );

    // Obtain information about th current process
    static uint32_t GetProcessInfo();

    // Helpers
    static uint32_t ConvertBytesToMiB( uint64_t bytes );
};

//------------------------------------------------------------------------------
