// CPUInfo.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Strings/AStackString.h"

//------------------------------------------------------------------------------
class CPUInfo
{
public:
    // Access processor information. Obtained on first call
    static const CPUInfo & Get();

    // Helper to get a core count that is useful for thread pool sizing
    // (ignores Low Power E-Cores for example)
    uint32_t GetNumUsefulCores() const;

    // Get a string containing details suitable for logging etc
    void GetCPUDetailsString( AString & outDetails ) const;

    // Processor Type info
    AStackString<48> m_CPUName;
#if defined( __x86_64__ ) || defined( _M_X64 )
    uint8_t m_Model = 0;
    uint8_t m_Family = 0;
    uint8_t m_Stepping = 0;
#endif

    // Totals
    uint32_t m_NumCores = 0; // Logical CPU cores
#if defined( __WINDOWS__ )
    uint32_t m_NumNUMANodes = 0; // Number of NUMA nodes
#endif

    // Per-Core Type
    uint32_t m_NumPCores = 0; // "Performance" cores
    uint32_t m_NumECores = 0; // "Efficiency" cores
    uint32_t m_NumLPECores = 0; // "Low Power Efficiency" cores

protected:
    CPUInfo();

    void DetermineProcessorType();
    void DetermineNumCores();
    void DetermineCoreTypes();
};

//------------------------------------------------------------------------------
