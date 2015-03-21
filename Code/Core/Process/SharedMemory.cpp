// SharedMemory
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Core/PrecompiledHeader.h"

#include "SharedMemory.h"

#if defined( __WINDOWS__ )
    #include <windows.h>
#endif

// CONSTRUCTOR
//------------------------------------------------------------------------------
SharedMemory::SharedMemory()
	: m_Memory( nullptr )
	#if defined( __WINDOWS__ )
		, m_MapFile( nullptr )
	#endif
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
SharedMemory::~SharedMemory()
{
	#if defined( __WINDOWS__ )
		if ( m_Memory )
		{
			UnmapViewOfFile( m_Memory );
		}
		if ( m_MapFile )
		{
			CloseHandle( m_MapFile );
		}
	#elif defined( __APPLE__ )
		// TODO:MAC Implement SharedMemory
	#elif defined( __LINUX__ )
		// TODO:LINUX Implement SharedMemory
	#else
		#error
	#endif
}

// Create
//------------------------------------------------------------------------------
void SharedMemory::Create( const char * name, unsigned int size )
{
	#if defined( __WINDOWS__ )
		m_MapFile = CreateFileMappingA( INVALID_HANDLE_VALUE,	// use paging file
										nullptr,				// default security
										PAGE_READWRITE,			// read/write access
										0,						// maximum object size (high-order DWORD)
										size,					// maximum object size (low-order DWORD)
										name );                 // name of mapping object
		if ( m_MapFile )
		{
			m_Memory = MapViewOfFile( m_MapFile,			// handle to map object
									  FILE_MAP_ALL_ACCESS,  // read/write permission
									  0,					// DWORD dwFileOffsetHigh
									  0,					// DWORD dwFileOffsetLow
									  size );
		}
	#elif defined( __APPLE__ )
		// TODO:MAC Implement SharedMemory
	#elif defined( __LINUX__ )
		// TODO:LINUX Implement SharedMemory
	#else
		#error
	#endif
}

// Open
//------------------------------------------------------------------------------
void SharedMemory::Open( const char * name, unsigned int size )
{
	#if defined( __WINDOWS__ )
		m_MapFile = OpenFileMappingA( FILE_MAP_ALL_ACCESS,	// read/write access
										FALSE,				// do not inherit the name
										name );               // name of mapping object
		if ( m_MapFile )
		{
			m_Memory = MapViewOfFile( m_MapFile,			// handle to map object
									  FILE_MAP_ALL_ACCESS,  // read/write permission
									  0,					// DWORD dwFileOffsetHigh
									  0,					// DWORD dwFileOffsetLow
									  size );
		}
	#elif defined( __APPLE__ )
		// TODO:MAC Implement SharedMemory
	#elif defined( __LINUX__ )
		// TODO:LINUX Implement SharedMemory
	#else
		#error
	#endif
}

//------------------------------------------------------------------------------
