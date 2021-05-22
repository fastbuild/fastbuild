// CompilerDriver_VBCC.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_QtRCC.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_QtRCC::CompilerDriver_QtRCC() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_QtRCC::~CompilerDriver_QtRCC() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_QtRCC::ProcessArg_PreprocessorOnly( const AString & token,
                                                                    size_t & index,
                                                                    const AString & /*nextToken*/,
                                                                    Args & /*outFullArgs*/ ) const
{
    // remove --output (or alias -o) so dependency list goes to stdout
    if ( StripTokenWithArg( "--output", token, index ) ||
         StripTokenWithArg( "-o", token, index ) )
    {
        return true;
    }

    return false;
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_QtRCC::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += " --list"; // List used resources
}

//------------------------------------------------------------------------------
