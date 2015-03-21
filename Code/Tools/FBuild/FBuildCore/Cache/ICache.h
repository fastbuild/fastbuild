// Cache - Cache interface
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_ICACHE_H
#define FBUILD_ICACHE_H

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Cache
//------------------------------------------------------------------------------
class ICache
{
public:
	inline virtual ~ICache() {}

	virtual bool Init( const AString & cachePath ) = 0;
	virtual void Shutdown() = 0;
	virtual bool Publish( const AString & cacheId, const void * data, size_t dataSize ) = 0;
	virtual bool Retrieve( const AString & cacheId, void * & data, size_t & dataSize ) = 0;
	virtual void FreeMemory( void * data, size_t dataSize ) = 0;
};

//------------------------------------------------------------------------------
#endif // FBUILD_ICACHE_H
