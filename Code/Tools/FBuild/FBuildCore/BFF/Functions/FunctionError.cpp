// FunctionError
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionError.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"

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
                                               BFFParser & /*parser*/,
                                               const BFFToken * functionNameStart,
                                               const BFFTokenRange & headerRange,
                                               const BFFTokenRange & /*bodyRange*/ ) const
{
    const BFFTokenRange iter( headerRange );

    // We expect a quoted string
    if ( iter->IsString() == false )
    {
        Error::Error_1001_MissingStringStartToken( iter.GetCurrent(), this );
        return false;
    }

    // perform variable substitutions
    AStackString< 1024 > message;
    if ( BFFParser::PerformVariableSubstitutions( iter.GetCurrent(), message ) == false )
    {
        return false; // substitution will have emitted an error
    }

    // Output user error
    Error::Error_1999_UserError( functionNameStart, this, message );

    return false; // Parsing is forced to fail
}

//------------------------------------------------------------------------------
