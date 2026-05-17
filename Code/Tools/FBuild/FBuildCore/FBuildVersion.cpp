// FBuildVersion.cpp
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "FBuildVersion.h"

// Core
#include <Core/Env/Assert.h>

//------------------------------------------------------------------------------
const char * GetVersionString()
{
    // Convert component parts into a string constant. We jump through these
    // hoops to have a string constant to ensure PrepareRelease.py can check
    // executables for the expected embedded string, in the format "vX.YY"
#define STRINGIFY( x ) #x
#define TOSTRING( x ) STRINGIFY(x)
    // clang-format off
    static const char * const sVersionString = "v"
                                               TOSTRING( FBUILD_VERSION_MAJOR )
                                               "."
                                               TOSTRING( FBUILD_VERSION_MINOR )
#if ( FBUILD_VERSION_EXT > 0 )
                                               "."
                                               TOSTRING( FBUILD_VERSION_EXT )
#endif
                                               FBUILD_VERSION_EXT_SUFFIX_STRING;
    // clang-format on
#undef STRINGIFY
#undef TOSTRING

    return sVersionString;
}

//------------------------------------------------------------------------------
uint32_t GetVersionIdentifier()
{
    ASSERT( FBUILD_VERSION_MINOR >= 1 );
    ASSERT( FBUILD_VERSION_MINOR <= 99 );
    return ( ( FBUILD_VERSION_MAJOR * 100 ) + FBUILD_VERSION_MINOR );
}

//------------------------------------------------------------------------------
