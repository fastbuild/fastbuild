// CPUInfo.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CPUInfo.h"

// Core
#include "Core/Env/Env.h"
#include "Core/Profile/Profile.h"

// External
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
    #include <lmcons.h>
    #include <stdio.h>
#endif

#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include <errno.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
#endif

#if defined( __APPLE__ )
    #include <sys/sysctl.h>
#endif

#if defined( __LINUX__ )
    #include <linux/limits.h>
    #include <pthread.h>
    #include <sched.h>
#endif

//------------------------------------------------------------------------------
namespace
{
    //------------------------------------------------------------------------------
#if defined( __x86_64__ ) || defined( _M_X64 )
    class CPUId
    {
    public:
        explicit CPUId( int32_t leaf, int32_t subLeaf = 0 );

        int32_t m_EAX;
        int32_t m_EBX;
        int32_t m_ECX;
        int32_t m_EDX;
    };
#endif

    //------------------------------------------------------------------------------
#if defined( __x86_64__ ) || defined( _M_X64 )
    CPUId::CPUId( int32_t leaf, int32_t subLeaf )
    {
        // References for cpuid:
        // - https://software.intel.com/content/www/us/en/develop/download/intel-architecture-instruction-set-extensions-programming-reference.html
    #if defined( __WINDOWS__ )
        // On Windows we can use the intrinsic
        __cpuidex( &m_EAX, leaf, subLeaf );
    #else
        // Other platforms must use inline asm
        __asm__ volatile( "cpuid"
                          : "=a"( m_EAX ), "=b"( m_EBX ), "=c"( m_ECX ), "=d"( m_EDX )
                          : "a"( leaf ), "c"( subLeaf )
                          : "cc" );
    #endif
    }
#endif
}

//------------------------------------------------------------------------------
namespace
{
#if defined( __WINDOWS__ ) || defined( __LINUX__ )
    //--------------------------------------------------------------------------
    uint64_t GetCurrentThreadAffinityMask()
    {
    #if defined( __WINDOWS__ )
        // Query requires we set it and it returns the old value
        const uint64_t mask = SetThreadAffinityMask( GetCurrentThread(), 0xFFFFFFFFFFFFFFFFULL );
        ASSERT( mask );

        // Restore the original mask
        SetThreadAffinityMask( GetCurrentThread(), mask );
    #else
        // Query the affinity via the cpu_set_t and build a mask
        uint64_t mask = 0;
        cpu_set_t cpuSet;
        CPU_ZERO( &cpuSet );
        VERIFY( sched_getaffinity( 0, sizeof( cpuSet ), &cpuSet ) == 0 );
        for ( size_t i = 0; i < 64; ++i )
        {
            if ( CPU_ISSET( i, &cpuSet ) )
            {
                mask |= ( 1 << i );
            }
        }
    #endif
        return mask;
    }

    //--------------------------------------------------------------------------
    void SetCurrentThreadAffinityMask( uint64_t mask )
    {
    #if defined( __WINDOWS__ )
        VERIFY( SetThreadAffinityMask( GetCurrentThread(), mask ) != 0 );
    #else
        cpu_set_t cpuSet;
        CPU_ZERO( &cpuSet );
        for ( size_t i = 0; i < 64; ++i )
        {
            if ( mask & ( 1 << i ) )
            {
                CPU_SET( i, &cpuSet );
            }
        }
        VERIFY( sched_setaffinity( 0, sizeof( cpuSet ), &cpuSet ) == 0 );
    #endif
    }
#endif
}

//------------------------------------------------------------------------------
uint32_t CPUInfo::GetNumUsefulCores() const
{
    // Performance and Efficiency cores can do useful work
    // Low Power Efficiency cores (m_LPECores) as seen in Intel Meteor Lake
    // CPUs won't be used for scheduling so should be ignored
    return ( m_NumCores - m_NumLPECores );
}

//------------------------------------------------------------------------------
void CPUInfo::GetCPUDetailsString( AString & outDetails ) const
{
    outDetails = m_CPUName;
#if defined( __x86_64__ ) || defined( _M_X64 )
    outDetails.AppendFormat( " (Family: 0x%02x Model: 0x%02x Stepping: 0x%02x)",
                             m_Family,
                             m_Model,
                             m_Stepping );
#endif
    outDetails.AppendFormat( " : %u Cores", m_NumCores );
#if defined( __WINDOWS__ )
    if ( m_NumNUMANodes > 0 )
    {
        outDetails.AppendFormat( " (%u NUMA nodes)", m_NumNUMANodes );
    }
#endif
    if ( m_NumCores != m_NumPCores )
    {
        outDetails.AppendFormat( " : %u P-Cores", m_NumPCores );
        if ( m_NumECores > 0 )
        {
            outDetails.AppendFormat( " + %u E-Cores", m_NumECores );
        }
        if ( m_NumLPECores > 0 )
        {
            outDetails.AppendFormat( " + %u Low Power E-Cores", m_NumLPECores );
        }
    }
}

//------------------------------------------------------------------------------
/*static*/ const CPUInfo & CPUInfo::Get()
{
    static CPUInfo sInfo; // Info is gathered on first call
    return sInfo;
}

//------------------------------------------------------------------------------
CPUInfo::CPUInfo()
{
    PROFILE_FUNCTION;

    // Obtain general info about the CPU (name etc)
    DetermineProcessorType();

    // Detect total core count
    DetermineNumCores();

    // Detect properties of individual cores, such as in Hybrid cores
    DetermineCoreTypes();

    ASSERT( m_NumCores == ( m_NumPCores + m_NumECores + m_NumLPECores ) );
}

//------------------------------------------------------------------------------
void CPUInfo::DetermineProcessorType()
{
    // We assume basics are the same for all cores

    // Intel platforms can use cpuid instructions. This also works on OSX under Rosetta.
#if defined( __x86_64__ ) || defined( _M_X64 )
    // CPUName
    {
        // If extended cpuid info is available, use the string from that
        // e.g. "AMD Ryzen 9 7950X3D 16-Core Processor"
        const CPUId cpuId80000000( static_cast<int32_t>( 0x80000000 ) );
        if ( cpuId80000000.m_EAX >= static_cast<int32_t>( 0x80000004 ) )
        {
            // Get the Processor Brand String which comes from
            char brandString[ ( 3 * 16 ) + 1 ] = { 0 };
            char * pos = brandString;
            for ( uint32_t i = 0x80000002; i <= 0x80000004; ++i )
            {
                const CPUId cpuId( static_cast<int32_t>( i ) );
                AString::Copy( reinterpret_cast<const char *>( &cpuId ), pos, sizeof( CPUId ) );
                pos += 16;
            }

            m_CPUName = brandString;
        }
        else
        {
            // Use the basic Vendor string as a fallback
            // e.g. "AuthenticAMD"

            // Note: string is swizzled
            const CPUId cpuId0( 0x00 );
            char vendorString[ 13 ] = { 0 };
            AString::Copy( reinterpret_cast<const char *>( &cpuId0.m_EBX ), vendorString + 0, 4 );
            AString::Copy( reinterpret_cast<const char *>( &cpuId0.m_EDX ), vendorString + 4, 4 );
            AString::Copy( reinterpret_cast<const char *>( &cpuId0.m_ECX ), vendorString + 8, 4 );
            m_CPUName = vendorString;
        }

        m_CPUName.TrimEnd( ' ' ); // Data from cpuid can be padded with trailing spaces
    }

    // Model/Family
    {
        const CPUId cpuId1( 0x01 );
        const uint8_t baseFamily = static_cast<uint8_t>( ( cpuId1.m_EAX >> 8 ) & 0xF );
        const uint8_t baseModel = static_cast<uint8_t>( ( cpuId1.m_EAX >> 4 ) & 0xF );

        // Family
        m_Family = baseFamily;
        if ( baseFamily == 0xF )
        {
            m_Family += static_cast<uint8_t>( ( cpuId1.m_EAX >> 20 ) & 0xFF );
        }

        // Model - NOTE: Extended model controlled by base family, not base model
        m_Model = baseModel;
        if ( ( baseFamily == 0x6 ) || ( baseFamily == 0xF ) )
        {
            m_Model += static_cast<uint8_t>( ( cpuId1.m_EAX >> 12 ) & 0xF0 );
        }

        // Stepping
        m_Stepping = static_cast<uint8_t>( cpuId1.m_EAX & 0xF );
    }
#else
    #if defined( __APPLE__ )
    char buffer[ 1024 ] = { 0 };
    size_t bufferSize = sizeof( buffer );
    if ( sysctlbyname( "machdep.cpu.brand_string", buffer, &bufferSize, nullptr, 0 ) == 0 )
    {
        m_CPUName = buffer;
    }
    else
    {
        m_CPUName = "Apple";
    }
    #else
    m_CPUName = "Unknown";
    #endif
#endif
}

//------------------------------------------------------------------------------
void CPUInfo::DetermineNumCores()
{
#if defined( __WINDOWS__ )
    // Default to NUMBER_OF_PROCESSORS
    m_NumCores = 1;

    AStackString<32> var;
    if ( Env::GetEnvVariable( "NUMBER_OF_PROCESSORS", var ) )
    {
        if ( var.Scan( "%u", &m_NumCores ) != 1 )
        {
            m_NumCores = 1;
        }
    }

    // As of Windows 7 -> Windows 10 / Windows Server 2016, the below information is valid:
    // On Systems with <= 64 Logical Processors, NUMBER_OF_PROCESSORS == "Logical Processor Count"
    // On Systems with >  64 Logical Processors, NUMBER_OF_PROCESSORS != "Logical Processor Count"
    // In the latter case, NUMBER_OF_PROCESSORS == "Logical Processor Count in this thread's NUMA Node"
    // So, we will check to see how many NUMA Nodes the system has here, and proceed accordingly:
    ULONG uNumNodes = 0;
    GetNumaHighestNodeNumber( &uNumNodes );
    m_NumNUMANodes = uNumNodes;

    if ( m_NumNUMANodes == 0 )
    {
        // The number of logical processors in the system all exist in one NUMA Node,
        // This means that NUMBER_OF_PROCESSORS represents the number of logical processors
        return;
    }

    // NUMBER_OF_PROCESSORS is incorrect for our system, so loop over all NUMA Nodes and accumulate logical core counts
    uint32_t numProcessorsInAllGroups = 0;
    for ( USHORT nodeID = 0; nodeID <= m_NumNUMANodes; ++nodeID )
    {
        GROUP_AFFINITY groupProcessorMask;
        memset( &groupProcessorMask, 0, sizeof( GROUP_AFFINITY ) );

        GetNumaNodeProcessorMaskEx( nodeID, &groupProcessorMask );

        // ULONG maxLogicalProcessorsInThisGroup = KeQueryMaximumProcessorCountEx( NodeID );
        // Each NUMA Node has a maximum of 32 cores on 32-bit systems and 64 cores on 64-bit systems
        const size_t maxLogicalProcessorsInThisGroup = sizeof( size_t ) * 8; // ( NumBits = NumBytes * 8 )
        uint32_t numProcessorsInThisGroup = 0;

        for ( size_t processorID = 0; processorID < maxLogicalProcessorsInThisGroup; ++processorID )
        {
            numProcessorsInThisGroup += ( ( groupProcessorMask.Mask &
                                            ( uint64_t( 1 ) << processorID ) ) != 0 )
                                            ? 1
                                            : 0;
        }

        numProcessorsInAllGroups += numProcessorsInThisGroup;
    }

    // If this ever fails, we might have encountered a weird new configuration
    ASSERT( numProcessorsInAllGroups >= m_NumCores );

    // If we computed more processors via NUMA Groups, use that number
    // Otherwise, fallback to returning NUMBER_OF_PROCESSORS
    // Even though we never expect the numa detection to be less, ensure we don't
    // behave badly by using NUMBER_OF_PROCESSORS in that case
    m_NumCores = Math::Max( numProcessorsInAllGroups, m_NumCores );
#else
    const long numCPUs = sysconf( _SC_NPROCESSORS_ONLN );
    if ( numCPUs <= 0 )
    {
        ASSERT( false ); // this should never fail
        m_NumCores = 1;
    }
    else
    {
        m_NumCores = static_cast<uint32_t>( numCPUs );
    }
#endif
}

//------------------------------------------------------------------------------
void CPUInfo::DetermineCoreTypes()
{
#if defined( __WINDOWS__ )
    // On systems with NUMA nodes, assume all cores are Performance cores
    // and use the existing detection logic which can returns CPU counts
    // greater than 64
    ULONG numNUMANodes = 0;
    VERIFY( GetNumaHighestNodeNumber( &numNUMANodes ) );
    m_NumNUMANodes = numNUMANodes;
    if ( numNUMANodes > 0 )
    {
        m_NumPCores = m_NumCores;
        return;
    }
#endif

#if defined( __APPLE__ )
    int32_t numPCores = 0;
    int32_t numECores = 0;
    size_t size = sizeof( int32_t );
    if ( ( sysctlbyname( "hw.perflevel0.physicalcpu", &numPCores, &size, nullptr, 0 ) == 0 ) &&
         ( sysctlbyname( "hw.perflevel1.physicalcpu", &numECores, &size, nullptr, 0 ) == 0 ) )
    {
        m_NumPCores = numPCores;
        m_NumECores = numECores;
        ASSERT( m_NumCores == ( m_NumPCores + m_NumECores ) );
    }
    else
    {
        // The above works on current Apple Silicon (even under Rosetta) but
        // may not work on older Intel CPUs or perhaps even early Apple Silicon
        // so in that case assume all cores are equivalent
        m_NumPCores = m_NumCores;
    }
    return;
#else
    // If we have more than 64 cores, assume they are all performance
    // cores because the logic to detect core types below relies on setting
    // affinities to each core which doesn't work with > 64 cores
    if ( m_NumCores > 64 )
    {
        m_NumPCores = m_NumCores;
        return;
    }

    // Detect Performance and Efficiency core breakdown

    // Introspect each core per Intel's guidance:
    //  - https://www.intel.com/content/www/us/en/developer/articles/guide/12th-gen-intel-core-processor-gamedev-guide.html
    // On AMD CPUs, all cores are detected as PCores currently as expected.
    // If AMD starts shipping SKUs with ECores, this function may require additional updates/logic.

    // Take note of the original affinity mask so we can restore it
    const uint64_t originalAffinity = GetCurrentThreadAffinityMask();

    enum class CoreType
    {
        ePCore,
        eECore,
        eLPECore,
    };

    // Iterate all the cores
    for ( uint32_t core = 0; core < m_NumCores; ++core )
    {
        // Switch affinity to core so cpuid function returns info about that core
        SetCurrentThreadAffinityMask( 1ULL << core );

        // Check if the core is on a "hybrid part"
        CoreType coreType = CoreType::ePCore;
        const CPUId cpuId7( 0x07 ); // Hybrid Part
        if ( cpuId7.m_EDX & ( 1 << 15 ) ) // Bit 15 in EDX
        {
            // Query core type
            const CPUId cpuId1A( 0x1A );// Core Type

            // Determine if this core is a PCore or ECore by checking
            // the top 8 bits in EAX
            if ( ( static_cast<uint32_t>( cpuId1A.m_EAX ) >> 24 ) == 0x20 ) // Intel Atom
            {
                coreType = CoreType::eECore;

                // Query cache info to detect Low Power E-Cores
                // as per: https://community.intel.com/t5/Mobile-and-Desktop-Processors/Detecting-LP-E-Cores-on-Meteor-Lake-in-software/m-p/1577956
                if ( ( m_Family == 0x6 ) &&
                     ( ( m_Model == 0xAA ) || ( m_Model == 0xAC ) ) )
                {
                    // Query cache type
                    const CPUId cpuId4( 0x4, 0x3 ); // Cache type
                    if ( cpuId4.m_EAX == 0 )
                    {
                        coreType = CoreType::eLPECore;
                    }
                }
            }
        }

        // Increment the appropriate core type
        switch ( coreType )
        {
            case CoreType::ePCore: m_NumPCores++; break;
            case CoreType::eECore: m_NumECores++; break;
            case CoreType::eLPECore: m_NumLPECores++; break;
        }
    }

    // Restore original affinity
    SetCurrentThreadAffinityMask( originalAffinity );
#endif
}

//------------------------------------------------------------------------------
