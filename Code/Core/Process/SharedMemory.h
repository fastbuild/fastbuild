// SharedMemory.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_PROCESS_SHAREDMEMORY_H
#define CORE_PROCESS_SHAREDMEMORY_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// SharedMemory
//------------------------------------------------------------------------------
class SharedMemory
{
public:
	SharedMemory();
	~SharedMemory();

	void Create( const char * name, unsigned int size );
	void Open( const char * name, unsigned int size );

	void * GetPtr() const { return m_Memory; }
private:
	void * m_Memory;
	#if defined( __WINDOWS__)
		void * m_MapFile;
	#endif
};

//------------------------------------------------------------------------------
#endif // CORE_PROCESS_SHAREDMEMORY_H
