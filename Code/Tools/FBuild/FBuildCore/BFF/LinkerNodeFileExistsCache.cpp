// LinkerNodeFileExistsCache
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "LinkerNodeFileExistsCache.h"

// Core
#include "Core/FileIO/FileIO.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
LinkerNodeFileExistsCache::LinkerNodeFileExistsCache() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
LinkerNodeFileExistsCache::~LinkerNodeFileExistsCache() = default;

// FileExists
//------------------------------------------------------------------------------
bool LinkerNodeFileExistsCache::FileExists( const AString & fileName )
{
    // Check the cache
    const auto * keyValue = m_FileExistsMap.Find( fileName );
    if ( keyValue )
    {
        return keyValue->m_Value; // Return previous result
    }

    // Checking for first time
    const bool exists = FileIO::FileExists( fileName.Get() );
    m_FileExistsMap.Insert( fileName, exists );
    return exists;
}

//------------------------------------------------------------------------------
