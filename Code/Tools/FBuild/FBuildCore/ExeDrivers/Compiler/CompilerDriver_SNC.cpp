// CompilerDriver_SNC.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_SNC.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_SNC::CompilerDriver_SNC() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_SNC::~CompilerDriver_SNC() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_SNC::ProcessArg_PreprocessorOnly( const AString & token,
                                                                  size_t & index,
                                                                  const AString & /*nextToken*/,
                                                                  Args & /*outFullArgs*/ ) const
{
    // Remove output arg
    if ( StripTokenWithArg( "-o", token, index ) )
    {
        return true;
    }

    // Remove "compile only" flag
    if ( StripToken( "-c", token ) )
    {
        return true;
    }

    return false;
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_SNC::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += "-E"; // run pre-processor only
}

//------------------------------------------------------------------------------
