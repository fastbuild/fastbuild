// Cache - Default cache implementation
//------------------------------------------------------------------------------

// Incldues
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Cache.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Mem/Mem.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
/*explicit*/ Cache::Cache()
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
/*virtual*/ Cache::~Cache()
{
}

// Init
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::Init( const AString & cachePath )
{
    PROFILE_FUNCTION

	m_CachePath = cachePath;
	PathUtils::EnsureTrailingSlash( m_CachePath );
	if ( FileIO::EnsurePathExists( m_CachePath ) )
	{
		return true;
	}

	FLOG_WARN( "Cache inaccessible - Caching disabled (Path '%s')", m_CachePath.Get() );
	return false;
}

// Shutdown
//------------------------------------------------------------------------------
/*virtual*/ void Cache::Shutdown()
{
	// Nothing to do
}

// Publish
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::Publish( const AString & cacheId, const void * data, size_t dataSize )
{
	AStackString<> cacheFileName;
	GetCacheFileName( cacheId, cacheFileName );

	// make sure the cache output path exists
	char * lastSlash = cacheFileName.FindLast( NATIVE_SLASH );
	*lastSlash = 0;
	if ( !FileIO::EnsurePathExists( cacheFileName ) )
	{
		return false;
	}
	*lastSlash = NATIVE_SLASH;

	// open output cache (tmp) file
	AStackString<> cacheFileTmpName( cacheFileName );
	cacheFileTmpName += ".tmp";
	FileStream cacheTmpFile;
	if( !cacheTmpFile.Open( cacheFileTmpName.Get(), FileStream::WRITE_ONLY ) )
	{
		return false;
	}

	// write data
	bool cacheTmpWriteOk = ( cacheTmpFile.Write( data, dataSize ) == dataSize );
	cacheTmpFile.Close();

	if ( !cacheTmpWriteOk )
	{
		// failed to write to cache tmp file
		FileIO::FileDelete( cacheFileTmpName.Get() ); // try to cleanup failure
		return false;
	}

	// rename tmp file to real file
	if ( FileIO::FileMove( cacheFileTmpName, cacheFileName ) == false )
	{
		// try to delete (possibly) existing file
		FileIO::FileDelete( cacheFileName.Get() );

		// try rename again
		if ( FileIO::FileMove( cacheFileTmpName, cacheFileName ) == false )
		{
			// problem renaming file
			FileIO::FileDelete( cacheFileTmpName.Get() ); // try to cleanup tmp file
			return false;
		}
	}

	return true;
}

// Retrieve
//------------------------------------------------------------------------------
/*virtual*/ bool Cache::Retrieve( const AString & cacheId, void * & data, size_t & dataSize )
{
	data = nullptr;
	dataSize = 0;

	AStackString<> cacheFileName;
	GetCacheFileName( cacheId, cacheFileName );

	FileStream cacheFile;
	if ( cacheFile.Open( cacheFileName.Get(), FileStream::READ_ONLY ) )
	{
		const size_t cacheFileSize = (size_t)cacheFile.GetFileSize();
		AutoPtr< char > mem( (char *)ALLOC( cacheFileSize ) );
		if ( cacheFile.Read( mem.Get(), cacheFileSize ) == cacheFileSize )
		{
			dataSize = cacheFileSize;
			data = mem.Release();
			return true;
		}
	}

	return false;
}

// FreeMemory
//------------------------------------------------------------------------------
/*virtual*/ void Cache::FreeMemory( void * data, size_t UNUSED( dataSize ) )
{
	FREE( data );
}

// GetCacheFileName
//------------------------------------------------------------------------------
void Cache::GetCacheFileName( const AString & cacheId, AString & path ) const
{
	// format example: N:\\fbuild.cache\\23\\77\\2377DE32_FED872A1_AB62FEAA23498AAC.3
	path.Format( "%s%c%c\\%c%c\\%s", m_CachePath.Get(), 
									   cacheId[ 0 ],
									   cacheId[ 1 ],
									   cacheId[ 2 ],
									   cacheId[ 3 ],
									   cacheId.Get() );
}

//------------------------------------------------------------------------------
