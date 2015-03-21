// Args
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_HELPERS_ARGS_H
#define FBUILD_HELPERS_ARGS_H

// Forward Declarations
//------------------------------------------------------------------------------
class AString;

// Includes
//------------------------------------------------------------------------------

// Args
//------------------------------------------------------------------------------
class Args
{
public:
	// helper functions
	static void StripQuotes( const char * start, const char * end, AString & out );

private:
};

//------------------------------------------------------------------------------
#endif // FBUILD_HELPERS_ARGS_H
