// Env - Provide host system details.
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_ENV_ENV_H
#define CORE_ENV_ENV_H

// Includes
//------------------------------------------------------------------------------
#include "Types.h"
#include "Core/Env/Assert.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Env
//------------------------------------------------------------------------------
class Env
{
public:
	enum Platform
	{
		WINDOWS,
		IOS,
		OSX,
		LINUX
	};

	static inline Platform GetPlatform();
	static inline const char * GetPlatformName( Platform platform );
	static inline const char * GetPlatformName() { return GetPlatformName( GetPlatform() ); }

	static uint32_t GetNumProcessors();

	static bool GetEnvVariable( const char * envVarName, AString & envVarValue );
	static void GetCmdLine( AString & cmdLine );
	static void GetExePath( AString & path );

	static uint32_t GetLastErr();
};

// GetPlatform
//------------------------------------------------------------------------------
/*static*/ inline Env::Platform Env::GetPlatform()
{
	#if defined( __WINDOWS__ )
		return Env::WINDOWS;
	#elif defined( __IOS__ )
		return Env::IOS;
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
		case Env::WINDOWS:	return "Windows";
		case Env::IOS:		return "IOS";
		case Env::OSX:		return "OSX";
		case Env::LINUX:	return "Linux";
	}
	ASSERT( false ); // should never get here
	return "Unknown";
}

//------------------------------------------------------------------------------
#endif // CORE_ENV_ENV_H
