// Env - Provide host system details.
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
template< class T > class Array;
class AString;

// Env
//------------------------------------------------------------------------------
class Env
{
public:
    enum Platform : uint8_t
    {
        WINDOWS,
        OSX,
        LINUX
    };

    static inline Platform GetPlatform();
    static inline const char * GetPlatformName( Platform platform );
    static inline const char * GetPlatformName() { return GetPlatformName( GetPlatform() ); }

    class ProcessorInfo
    {
    public:
        ProcessorInfo();

        uint32_t mNumCores = 0;     // Logical CPU cores
        uint32_t mNumPCores = 0;    // "Performance" cores
        uint32_t mNumECores = 0;    // "Efficiency" cores
    };
    static const ProcessorInfo & GetProcessorInfo();
    static uint32_t GetNumProcessors();

    static bool GetEnvVariable( const char * envVarName, AString & envVarValue );
    static bool SetEnvVariable( const char * envVarName, const AString & envVarValue );
    static void GetCmdLine( AString & cmdLine );
    static void GetExePath( AString & path );
    static bool IsStdOutRedirected( const bool recheck = false );
    static bool GetLocalUserName( AString & outUserName );

    static uint32_t GetLastErr();
    static const char * AllocEnvironmentString( const Array< AString > & environment );
    static void ShowMsgBox( const char * title, const char * msg );
};

// GetPlatform
//------------------------------------------------------------------------------
/*static*/ inline Env::Platform Env::GetPlatform()
{
    #if defined( __WINDOWS__ )
        return Env::WINDOWS;
    #elif defined( __OSX__ )
        return Env::OSX;
    #elif defined( __LINUX__ )
        return Env::LINUX;
    #endif
}

// GetPlatformName
//------------------------------------------------------------------------------
/*static*/ inline const char * Env::GetPlatformName( Platform platform )
{
    switch ( platform )
    {
        case Env::WINDOWS:  return "Windows";
        case Env::OSX:      return "OSX";
        case Env::LINUX:    return "Linux";
    }
    ASSERT( false ); // should never get here
    return "Unknown";
}

//------------------------------------------------------------------------------
