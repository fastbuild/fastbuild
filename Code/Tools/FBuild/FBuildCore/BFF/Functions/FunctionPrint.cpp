// FunctionPrint
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionPrint.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionPrint::FunctionPrint()
: Function( "Print" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::AcceptsHeader() const
{
	return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::NeedsHeader() const
{
	return true;
}

// NeedsBody
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::NeedsBody() const
{
	return false;
}

//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::ParseFunction( const BFFIterator & functionNameStart,
											   const BFFIterator * functionBodyStartToken, 
											   const BFFIterator * functionBodyStopToken,
											   const BFFIterator * functionHeaderStartToken,
											   const BFFIterator * functionHeaderStopToken ) const
{
	(void)functionNameStart;
	(void)functionBodyStartToken;
	(void)functionBodyStopToken;

	if ( functionHeaderStartToken && functionHeaderStopToken && 
		 ( functionHeaderStartToken->GetDistTo( *functionHeaderStopToken ) >= 1 ) )
	{
		BFFIterator start( *functionHeaderStartToken );
		ASSERT( *start == BFFParser::BFF_FUNCTION_ARGS_OPEN );
		start++;
		start.SkipWhiteSpace();

		// a quoted string?
		const char c = *start;
		if ( ( c != '"' ) && ( c != '\'' ) )
		{
			Error::Error_1001_MissingStringStartToken( start, this ); 
			return false;
		}

		// find end of string
		BFFIterator stop( start );
		stop.SkipString( c );
		ASSERT( stop.GetCurrent() <= functionHeaderStopToken->GetCurrent() ); // should not be in this function if strings are not validly terminated

		// perform variable substitutions
		AStackString< 1024 > tmp;

		start++; // skip past opening quote
		if ( BFFParser::PerformVariableSubstitutions( start, stop, tmp ) == false )
		{
			return false; // substitution will have emitted an error
		}
		tmp += '\n';

		FLOG_BUILD_DIRECT( tmp.Get() );
	}

	return true;
}

//------------------------------------------------------------------------------
