// FBuildVersion.h
//------------------------------------------------------------------------------
#pragma once

// Defines
//------------------------------------------------------------------------------
// FASTBuild controlled versions
#define FBUILD_VERSION_MAJOR 1
#define FBUILD_VERSION_MINOR 17

// Version extensions reserved for 3rd party use
#if !defined( FBUILD_VERSION_EXT )
    #define FBUILD_VERSION_EXT 0
#endif
#if !defined( FBUILD_VERSION_EXT_SUFFIX_STRING )
    #define FBUILD_VERSION_EXT_SUFFIX_STRING ""
#endif

// Platform
#if defined( __WINDOWS__ )
    #define FBUILD_VERSION_PLATFORM "Windows"
#elif defined( __APPLE__ )
    #define FBUILD_VERSION_PLATFORM "OSX"
#elif defined( __LINUX__ )
    #define FBUILD_VERSION_PLATFORM "Linux"
#else
    #error Unknown platform
#endif

// Includes
//------------------------------------------------------------------------------
#include <Core/Env/Types.h>

//------------------------------------------------------------------------------
// Get the full version string.
//  Default                 : "vX.YY"
//  With extensions (if set): "vX.YY.Zuser"
const char * GetVersionString();

// Return version used by _FASTBUILD_VERSION_ and network protocol, derived
// from MAJOR and MINOR version only
//  e.g. XYY
uint32_t GetVersionIdentifier();

//------------------------------------------------------------------------------
