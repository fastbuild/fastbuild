// MemTracker.h
//------------------------------------------------------------------------------
#pragma once

// Enabled in DEBUG only
//------------------------------------------------------------------------------
#ifdef DEBUG
    #define MEMTRACKER_ENABLED
#endif

// MemTracker
//------------------------------------------------------------------------------
#if !defined( MEMTRACKER_ENABLED )
    #define MEMTRACKER_ALLOC( ptr, size, file, line ) (void)0
    #define MEMTRACKER_FREE( ptr ) (void)0
    #define MEMTRACKER_DUMP_ALLOCATIONS
    #define MEMTRACKER_DISABLE_THREAD
    #define MEMTRACKER_ENABLE_THREAD
#else
    // Includes
    //------------------------------------------------------------------------------
    #include "Core/Env/Types.h"
    #include "Core/Process/Mutex.h"

    // Forward Declarations
    //------------------------------------------------------------------------------
    class MemPoolBlock;

    // Macros
    //------------------------------------------------------------------------------
    #define MEMTRACKER_ALLOC( ptr, size, file, line )   MemTracker::Alloc( ptr, size, file, line )
    #define MEMTRACKER_FREE( ptr )                      MemTracker::Free( ptr )
    #define MEMTRACKER_DUMP_ALLOCATIONS                 MemTracker::DumpAllocations();
    #define MEMTRACKER_DISABLE_THREAD                   MemTracker::DisableOnThread();
    #define MEMTRACKER_ENABLE_THREAD                    MemTracker::EnableOnThread();

    // MemTracker
    //------------------------------------------------------------------------------
    class MemTracker
    {
    public:
        static void Alloc( void * ptr, size_t size, const char * file, int line );
        static void Free( void * ptr );

        static void DisableOnThread();
        static void EnableOnThread();

        static void Reset();
        static void DumpAllocations();

        static inline uint32_t GetCurrentAllocationCount() { return s_AllocationCount; }
        static inline uint32_t GetCurrentAllocationId() { return s_Id; }

        struct Allocation
        {
            void *          m_Ptr;
            size_t          m_Size;
            Allocation *    m_Next;
            const char *    m_File;
            uint32_t        m_Line;
            uint32_t        m_Id;
        };
    private:
        static void Init();

        static Mutex & GetMutex() { return reinterpret_cast< Mutex & >( s_Mutex ); }

        static uint32_t         s_Id;
        static bool             s_Enabled;
        static volatile bool    s_Initialized;
        static uint32_t         s_AllocationCount;
        static Allocation *     s_LastAllocation;
        static uint64_t         s_Mutex[ sizeof( Mutex ) / sizeof( uint64_t ) ];
        static Allocation **    s_AllocationHashTable;
        static MemPoolBlock *   s_Allocations;
    };

#endif // MEMTRACKER_ENABLED

//------------------------------------------------------------------------------
