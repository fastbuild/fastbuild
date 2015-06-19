// FBuildVersion.h
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FBUILDVERSION_H
#define FBUILD_FBUILDVERSION_H

// Defines
//------------------------------------------------------------------------------
#define FBUILD_VERSION_STRING "v0.83"
#if defined( __WINDOWS__ )
	#ifdef WIN64
		#define FBUILD_VERSION_PLATFORM "x64"
	#else
		#define FBUILD_VERSION_PLATFORM "x86"
	#endif
#elif defined( __APPLE__ ) || defined( __LINUX__ )
	#ifdef __x86_64__
		#define FBUILD_VERSION_PLATFORM "x64"
	#else
		#define FBUILD_VERSION_PLATFORM "x86"
	#endif
#else
	#error Unknown platform
#endif

//------------------------------------------------------------------------------
#endif // FBUILD_FBUILDVERSION_H
