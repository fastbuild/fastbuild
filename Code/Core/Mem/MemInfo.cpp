// MemInfo.cpp
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MemInfo.h"

// Core
#include "Core/Env/Assert.h"
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h" // Must be before Psapi.h
#endif

// System
#if defined( __WINDOWS__ )
    #include <Psapi.h>
#else
    #if defined( __OSX__ )
        #include <mach/mach.h>
        #include <mach/vm_statistics.h>
        #include <sys/sysctl.h>
    #endif
    #include <unistd.h>
#endif

// GetMemInfo
//------------------------------------------------------------------------------
/*static*/ void MemInfo::GetSystemInfo( SystemMemInfo & outInfo )
{
    // Get general system info
    #if defined( __WINDOWS__ )
        PERFORMANCE_INFORMATION info;
        info.cb = sizeof( info );
        VERIFY( GetPerformanceInfo( &info, sizeof( info ) ) );
        const uint64_t pageSize = info.PageSize;
        const uint64_t physTotalPages = info.PhysicalTotal;
        const uint64_t physAvailPages = info.PhysicalAvailable;
    #else
    
        const uint64_t pageSize = static_cast<uint64_t>( sysconf( _SC_PAGE_SIZE ) );
        const uint64_t physTotalPages = static_cast<uint64_t>( sysconf( _SC_PHYS_PAGES ) );
        #if defined( __LINUX__ )
            const uint64_t physAvailPages = static_cast<uint64_t>( sysconf( _SC_AVPHYS_PAGES ) );
        #else
            // NOTE: OSX doesn't support _SC_AVPHYS_PAGES so we have to obtain
            //       the value elsewhere. Unfortunately Apple doesn't document
            //       how any of this works or how these numbers should be
            //       interpretted so this is a best guess.
            mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
            vm_statistics64_data_t vmstat;
            VERIFY( host_statistics64( mach_host_self(),
                                       HOST_VM_INFO,
                                       (host_info_t)&vmstat,
                                       &count ) == KERN_SUCCESS);
            const uint64_t physAvailPages = ( vmstat.free_count +
                                              vmstat.inactive_count +
                                              vmstat.speculative_count );
        #endif
    #endif
    // Physical memory accessible by OS in MiB
    outInfo.mTotalPhysMiB = ConvertBytesToMiB( physTotalPages * pageSize );

    // Physical free memory
    outInfo.mAvailPhysMiB = ConvertBytesToMiB( physAvailPages * pageSize );
}

//------------------------------------------------------------------------------
/*static*/ uint32_t MemInfo::ConvertBytesToMiB( uint64_t bytes )
{
    const uint64_t kMebibyte = ( 1024 * 1024 );
    return static_cast<uint32_t>( bytes / kMebibyte );
}

//------------------------------------------------------------------------------
