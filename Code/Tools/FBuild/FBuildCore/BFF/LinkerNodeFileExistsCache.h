// LinkerNodeFileExistsCache
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Singleton.h"
#include "Core/Containers/UnorderedMap.h"
#include "Core/Strings/AString.h"

// LinkerNodeFileExistsCache
//------------------------------------------------------------------------------
class LinkerNodeFileExistsCache : public Singleton< LinkerNodeFileExistsCache >
{
public:
    explicit LinkerNodeFileExistsCache();
    ~LinkerNodeFileExistsCache();

    bool FileExists( const AString & fileName );

    LinkerNodeFileExistsCache & operator = ( const LinkerNodeFileExistsCache & other ) = delete;

private:
    UnorderedMap< AString, bool > m_FileExistsMap;
};

//------------------------------------------------------------------------------
