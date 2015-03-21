// Args
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Args.h"

// Core
#include "Core/Strings/AString.h"

// Generate
//------------------------------------------------------------------------------
/*static*/ void Args::StripQuotes( const char * start, const char * end, AString & out )
{
	ASSERT( start );
	ASSERT( end );

	// handle empty inputs
	if ( start == end )
	{
		out.Clear();
		return;
	}

	// strip first quote if there is one
	const char firstChar = *start;
	if ( ( firstChar == '"' ) || ( firstChar == '\'' ) )
	{
		++start;
	}

	// strip first quote if there is one
	const char lastChar = *( end - 1 );
	if ( ( lastChar == '"' ) || ( lastChar == '\'' ) )
	{
		--end;
	}

	// handle invalid strings (i.e. just one quote)
	if ( end < start )
	{
		out.Clear();
		return;
	}

	// assign unquoted string (could be empty, and that's ok)
	out.Assign( start, end );
}

//------------------------------------------------------------------------------
