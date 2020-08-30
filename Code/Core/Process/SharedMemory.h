// SharedMemory.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"
#if defined( __LINUX__ ) || defined( __APPLE__ )
    #include "Core/Strings/AString.h"
#endif

// SharedMemory
//------------------------------------------------------------------------------
class SharedMemory
{
public:
    SharedMemory();
    ~SharedMemory();

    void Create( const char * name, unsigned int size );
    bool Open( const char * name, unsigned int size );

    void * GetPtr() const { return m_Memory; }

private:
    friend class TestSharedMemory;
    void Unmap(); // Used in unit tests

    void * m_Memory;
    #if defined(  __WINDOWS__ )
        void * m_MapFile;
    #elif defined( __LINUX__ ) || defined( __APPLE__ )
        int m_MapFile;
        size_t m_Length;
        AString m_Name;
    #else
        #error Unknown Platform
    #endif
};

//------------------------------------------------------------------------------
