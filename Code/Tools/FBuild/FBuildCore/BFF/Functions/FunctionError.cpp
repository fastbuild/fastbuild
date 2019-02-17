// FunctionError
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionError.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionError::FunctionError()
: Function( "Error" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionError::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionError::NeedsHeader() const
{
    return true;
}

// NeedsBody
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionError::NeedsBody() const
{
    return false;
}

// ParseFunction
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionError::ParseFunction( NodeGraph & /*nodeGraph*/,
                                               const BFFIterator & functionNameStart,
                                               const BFFIterator * /*functionBodyStartToken*/,
                                               const BFFIterator * /*functionBodyStopToken*/,
                                               const BFFIterator * functionHeaderStartToken,
                                               const BFFIterator * functionHeaderStopToken ) const
{
    ASSERT( functionHeaderStartToken && functionHeaderStopToken ); (void)functionHeaderStopToken;

    // Start parsing function header
    BFFIterator pos( *functionHeaderStartToken );
    ASSERT( *pos == BFFParser::BFF_FUNCTION_ARGS_OPEN );
    pos++; // Skip opening
    pos.SkipWhiteSpace();

    // We expect a quoted string
    if ( !pos.IsAtString() )
    {
        Error::Error_1001_MissingStringStartToken( pos, this );
        return false;
    }
    BFFIterator messageStart( pos );
    messageStart++; // Skip opening quote

    // Find end of string
    pos.SkipString();
    ASSERT( pos.GetCurrent() <= functionHeaderStopToken->GetCurrent() ); // should not be in this function if strings are not validly terminated
    ASSERT( pos.IsAtString() );
    const BFFIterator messageStop( pos );

    // Make sure there are no extraneous tokens
    pos++; // Closing quote
    pos.SkipWhiteSpace();
    if ( *pos != ')' )
    {
        Error::Error_1002_MatchingClosingTokenNotFound( pos, this, ')' );
        return false;
    }

    // perform variable substitutions
    AStackString< 1024 > message;
    if ( BFFParser::PerformVariableSubstitutions( messageStart, messageStop, message ) == false )
    {
        return false; // substitution will have emitted an error
    }

    // Output user error
    Error::Error_1999_UserError( functionNameStart, this, message );

    return false; // Parsing is forced to fail
}

//------------------------------------------------------------------------------
