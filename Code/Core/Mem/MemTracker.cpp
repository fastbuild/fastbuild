// MemTracker.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "MemTracker.h"
#include "Core/Env/Types.h"

#if defined( __WINDOWS__ )
    PRAGMA_DISABLE_PUSH_MSVC( 4073 ) // initializers put in library initialization area
    #pragma init_seg(lib)
    PRAGMA_DISABLE_POP_MSVC
#endif

//------------------------------------------------------------------------------
#if defined( MEMTRACKER_ENABLED )
    // Includes
    //------------------------------------------------------------------------------
    #include "Core/Mem/MemPoolBlock.h"
    #include "Core/Process/Atomic.h"
    #include "Core/Process/Thread.h"
    #include "Core/Tracing/Tracing.h"

    // system
    #include <memory.h> // for memset

    // GlobalData
    //------------------------------------------------------------------------------
    class LeakDumper { public: ~LeakDumper() { MemTracker::DumpAllocations(); } };
    #if defined( __WINDOWS__ ) && !defined( __clang__ )
        static LeakDumper g_LeakDumper;
    #else
        static LeakDumper g_LeakDumper __attribute__((init_priority(101)));
    #endif

    // Static Data
    //------------------------------------------------------------------------------
    /*static*/ uint32_t         MemTracker::s_Id( 0 );
    /*static*/ bool             MemTracker::s_Enabled( true );
    /*static*/ volatile bool    MemTracker::s_Initialized( false );
    /*static*/ uint32_t         MemTracker::s_AllocationCount( 0 );
    /*static*/ uint64_t         MemTracker::s_Mutex[];
    /*static*/ MemTracker::Allocation ** MemTracker::s_AllocationHashTable = nullptr;
    /*static*/ MemPoolBlock *   MemTracker::s_Allocations( nullptr );

    // Thread-Local Data
    //------------------------------------------------------------------------------
    THREAD_LOCAL uint32_t       g_MemTrackerDisabledOnThisThread( 0 );

    // Defines
    #define ALLOCATION_HASH_SHIFT       ( 0x2 )         // shift off lower bits
    #define ALLOCATION_HASH_SIZE        ( 0x100000 )    // one megabyte
    #define ALLOCATION_HASH_MASK        ( 0x0FFFFF )    // mask off upper bits

    // Alloc
    //------------------------------------------------------------------------------
    /*static*/ void MemTracker::Alloc( void * ptr, size_t size, const char * file, int line )
    {
        if ( !s_Enabled )
        {
            return;
        }

        // handle allocations during initialization
        if ( g_MemTrackerDisabledOnThisThread )
        {
            return;
        }

        ++g_MemTrackerDisabledOnThisThread;

        if ( !s_Initialized )
        {
            Init();
        }

        const size_t hashIndex = ( ( (size_t)ptr >> ALLOCATION_HASH_SHIFT ) & ALLOCATION_HASH_MASK );

        {
            MutexHolder mh( GetMutex() );

            Allocation * a = (Allocation *)s_Allocations->Alloc();
            ++s_AllocationCount;

            a->m_Id = ++s_Id;
            a->m_Ptr = ptr;
            a->m_Size = size;
            a->m_Next = s_AllocationHashTable[ hashIndex ];
            a->m_File = file;
            a->m_Line = (uint32_t)line;
            static size_t breakOnSize = (size_t)-1;
            static uint32_t breakOnId = 0;
            if ( ( size == breakOnSize ) || ( a->m_Id == breakOnId ) )
            {
                BREAK_IN_DEBUGGER;
            }

            s_AllocationHashTable[ hashIndex ] = a;
        }

        --g_MemTrackerDisabledOnThisThread;
    }

    // Free
    //------------------------------------------------------------------------------
    /*static*/ void MemTracker::Free( void * ptr )
    {
        if ( !s_Enabled )
        {
            return;
        }

        if ( !s_Initialized )
        {
            return;
        }

        if ( g_MemTrackerDisabledOnThisThread )
        {
            return;
        }

        const size_t hashIndex = ( ( (size_t)ptr >> ALLOCATION_HASH_SHIFT ) & ALLOCATION_HASH_MASK );

        MutexHolder mh( GetMutex() );

        Allocation * a = s_AllocationHashTable[ hashIndex ];
        Allocation * prev = nullptr;
        while ( a )
        {
            if ( a->m_Ptr == ptr )
            {
                if ( prev == nullptr )
                {
                    s_AllocationHashTable[ hashIndex ] = a->m_Next;
                }
                else
                {
                    prev->m_Next = a->m_Next;
                }
                ++g_MemTrackerDisabledOnThisThread;
                s_Allocations->Free( a );
                --s_AllocationCount;
                --g_MemTrackerDisabledOnThisThread;
                break;
            }
            prev = a;
            a = a->m_Next;
        }
    }

    // DisableOnThread
    //------------------------------------------------------------------------------
    /*static*/ void MemTracker::DisableOnThread()
    {
        ++g_MemTrackerDisabledOnThisThread;
    }

    // EnableOnThread
    //------------------------------------------------------------------------------
    /*static*/ void MemTracker::EnableOnThread()
    {
        --g_MemTrackerDisabledOnThisThread;
    }

    // DumpAllocations
    //------------------------------------------------------------------------------
    /*static*/ void MemTracker::DumpAllocations()
    {
        if ( s_Enabled == false )
        {
            OUTPUT( "DumpAllocations failed - MemTracker not enabled\n" );
            return;
        }

        if ( s_Initialized == false )
        {
            OUTPUT( "DumpAllocations : No allocations\n" );
            return;
        }

        MutexHolder mh( GetMutex() );

        if ( s_AllocationCount == 0 )
        {
            OUTPUT( "DumpAllocations : No allocations\n" );
            return;
        }

        uint64_t total = 0;
        uint64_t numAllocs = 0;

        // for each leak, we'll print a view of the memory
        char displayChar[256];
        memset( displayChar, '.', sizeof( displayChar ) );
        const char * okChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~`1234567890-=!@#$^&*()_+[]{};:'\",<>/?|\\";
        const char * ok = okChars;
        for ( ;; ) { char c = *ok; if ( c == 0 ) break; displayChar[ (unsigned char)c ] = c; ++ok; }

        char memView[ 32 ] = { 0 };

        OUTPUT( "--- DumpAllocations ------------------------------------------------\n" );
        for ( size_t i=0; i<ALLOCATION_HASH_SIZE; ++i )
        {
            Allocation * a = s_AllocationHashTable[ i ];
            while ( a )
            {
                const uint32_t id     = a->m_Id;
                const uint64_t addr   = (size_t)a->m_Ptr;
                const uint64_t size   = a->m_Size;

                // format a view of the memory contents
                const char * src = (const char *)addr;
                char * dst = memView;
                const size_t num = Math::Min< size_t >( (size_t)size, 31 );
                for ( uint32_t j=0; j<num; ++j )
                {
                    char c = *src;
                    *dst = displayChar[ (uint8_t)c ];
                    ++src;
                    ++dst;
                }
                *dst = 0;

                OUTPUT( "%s(%u): Id %u : %" PRIu64 " bytes @ 0x%016" PRIx64 " (Mem: %s)\n", a->m_File, a->m_Line, id, size, addr, memView );

                ++numAllocs;
                total += size;

                a = a->m_Next;
            }
        }
        OUTPUT( "--------------------------------------------------------------------\n" );
        OUTPUT( "Total: %" PRIu64 " bytes in %" PRIu64 " allocs\n", total, numAllocs );
        OUTPUT( "--------------------------------------------------------------------\n" );
    }

    // Reset
    //------------------------------------------------------------------------------
    /*static*/ void MemTracker::Reset()
    {
        MutexHolder mh( GetMutex() );

        ++g_MemTrackerDisabledOnThisThread;

        // free all allocation tracking
        for ( size_t i=0; i<ALLOCATION_HASH_SIZE; ++i )
        {
            Allocation * a = s_AllocationHashTable[ i ];
            while ( a )
            {
                s_Allocations->Free( a );
                --s_AllocationCount;
                a = a->m_Next;
            }
            s_AllocationHashTable[ i ] = nullptr;
        }

        ASSERT( s_AllocationCount == 0 );

        s_Id = 0;

        --g_MemTrackerDisabledOnThisThread;
    }

    // Init
    //------------------------------------------------------------------------------
    /*static*/ void MemTracker::Init()
    {
        static_assert( sizeof( MemTracker::s_Mutex ) == sizeof( Mutex ), "Unexpected sizeof(Mutex)" );

        ASSERT( g_MemTrackerDisabledOnThisThread );

        // first caller does init
        static uint32_t threadSafeGuard( 0 );
        if ( AtomicIncU32( &threadSafeGuard ) != 1 )
        {
            // subsequent callers wait for init
            while ( !s_Initialized ) {}
            return;
        }

        // construct primary mutex in-place
        INPLACE_NEW ( &GetMutex() ) Mutex;

        // init hash table
        s_AllocationHashTable = new Allocation*[ ALLOCATION_HASH_SIZE ];
        memset( s_AllocationHashTable, 0, ALLOCATION_HASH_SIZE * sizeof( Allocation * ) );

        // init pool for allocation structures
        s_Allocations = new MemPoolBlock( sizeof( Allocation ), __alignof( Allocation ) );

        MemoryBarrier();

        s_Initialized = true;
    }

//------------------------------------------------------------------------------
#endif // MEMTRACKER_ENABLED

//------------------------------------------------------------------------------
