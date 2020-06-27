// SharedMemory
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "SharedMemory.h"
#include "Core/Env/Assert.h"
#include "Core/Strings/AString.h"

#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#elif defined( __LINUX__ ) || defined( __APPLE__ )
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <unistd.h>
#endif

#if defined( __LINUX__ ) || defined( __APPLE__ )
namespace
{
bool PosixMapMemory( const char * name,
                     size_t length,
                     bool create,
                     int * mapFile,
                     void ** memory,
                     AString & portableName )
{
    ASSERT( *mapFile == -1 );
    ASSERT( *memory == nullptr );

    // From man shm_open :
    // For portable use, a shared memory object should be identified by a
    // name of the form /somename; that is, a null-terminated string of up
    // to NAME_MAX (i.e., 255) characters consisting  of an initial slash,
    // followed by one or more characters, none of which are slashes.
    // For OSX compatibility, name must also be shorter than SHM_NAME_MAX (32)
    portableName = "/";
    portableName += name;
    ASSERT( portableName.FindLast('/') == portableName.Get() );
    ASSERT( portableName.GetLength() <= 32 ); // from SHM_NAME_MAX on OSX

    *mapFile = shm_open( portableName.Get(),
                         O_RDWR | (create ? O_CREAT : 0),
                         S_IWUSR | S_IRUSR );
    if ( *mapFile == -1 )
    {
        return false;
    }

    if ( create )
    {
        VERIFY( ftruncate( *mapFile, length ) == 0 );
    }

    *memory = mmap( nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, *mapFile, 0 );
    return ( *memory != MAP_FAILED );
}
}
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
SharedMemory::SharedMemory()
    : m_Memory( nullptr )
    #if defined( __WINDOWS__ )
        , m_MapFile( nullptr )
    #elif defined(__LINUX__) || defined(__APPLE__)
        , m_MapFile( -1 )
        , m_Length( 0 )
    #else
        #error Unknown Platform
    #endif
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SharedMemory::~SharedMemory()
{
    Unmap();
    #if defined( __WINDOWS__ )
        if ( m_MapFile )
        {
            CloseHandle( m_MapFile );
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        if ( m_MapFile != -1 )
        {
            close( m_MapFile );
            shm_unlink( m_Name.Get() );
        }
    #else
        #error Unknown Platform
    #endif
}

// Create
//------------------------------------------------------------------------------
void SharedMemory::Create( const char * name, unsigned int size )
{
    #if defined( __WINDOWS__ )
        ASSERT( m_MapFile == nullptr );
        ASSERT( m_Memory == nullptr );
        m_MapFile = CreateFileMappingA( INVALID_HANDLE_VALUE,   // use paging file
                                        nullptr,                // default security
                                        PAGE_READWRITE,         // read/write access
                                        0,                      // maximum object size (high-order DWORD)
                                        size,                   // maximum object size (low-order DWORD)
                                        name );                 // name of mapping object
        if ( m_MapFile )
        {
            m_Memory = MapViewOfFile( m_MapFile,            // handle to map object
                                      FILE_MAP_ALL_ACCESS,  // read/write permission
                                      0,                    // DWORD dwFileOffsetHigh
                                      0,                    // DWORD dwFileOffsetLow
                                      size );
        }
    #elif defined( __APPLE__ ) || defined( __LINUX__ )
        PosixMapMemory(name, size, true, &m_MapFile, &m_Memory, m_Name);
        m_Length = size;
    #else
        #error Unknown Platform
    #endif
}

// Open
//------------------------------------------------------------------------------
bool SharedMemory::Open( const char * name, unsigned int size )
{
    #if defined( __WINDOWS__ )
        m_MapFile = OpenFileMappingA( FILE_MAP_ALL_ACCESS,  // read/write access
                                        FALSE,              // do not inherit the name
                                        name );             // name of mapping object
        if ( m_MapFile )
        {
            m_Memory = MapViewOfFile( m_MapFile,            // handle to map object
                                      FILE_MAP_ALL_ACCESS,  // read/write permission
                                      0,                    // DWORD dwFileOffsetHigh
                                      0,                    // DWORD dwFileOffsetLow
                                      size );
        }
        return ( ( m_Memory != nullptr ) && ( m_MapFile != nullptr ) );
    #elif defined( __APPLE__ ) || defined(__LINUX__)
        const bool result = PosixMapMemory(name, size, false, &m_MapFile, &m_Memory, m_Name);
        m_Length = size;
        return result;
    #else
        #error
    #endif
}

// Unmap
//------------------------------------------------------------------------------
void SharedMemory::Unmap()
{
    #if defined( __WINDOWS__ )
        if ( m_Memory )
        {
            UnmapViewOfFile( m_Memory );
            m_Memory = nullptr;
        }
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        if ( m_Memory )
        {
            ASSERT( m_Length > 0 );
            munmap( m_Memory, m_Length );
            m_Memory = nullptr;
        }
    #else
        #error Unknown Platform
    #endif
}

//------------------------------------------------------------------------------
