// ICache - Cache interface
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "ICache.h"

#include <Core/Strings/AString.h>

// GetCacheId
//------------------------------------------------------------------------------
/*static*/ void ICache::GetCacheId( const uint64_t preprocessedSourceKey,
                                    const uint32_t commandLineKey,
                                    const uint64_t toolChainKey,
                                    const uint64_t pchKey,
                                    AString & outCacheId )
{
    // cache version - bump if cache format is changed
    static const char cacheVersion( 'B' );

    // format example: 2377DE32AB045A2D_FED872A1_AB62FEAA23498AAC-32A2B04375A2D7DE.7
    outCacheId.Format( "%016" PRIX64 "_%08X_%016" PRIX64 "-%016" PRIX64 ".%c",
                       preprocessedSourceKey,
                       commandLineKey,
                       toolChainKey,
                       pchKey,
                       cacheVersion );
}

//------------------------------------------------------------------------------
