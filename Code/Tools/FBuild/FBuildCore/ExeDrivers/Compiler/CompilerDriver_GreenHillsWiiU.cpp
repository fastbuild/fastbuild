// CompilerDriver_GreenHillsWiiU.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_GreenHillsWiiU.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_GreenHillsWiiU::CompilerDriver_GreenHillsWiiU() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_GreenHillsWiiU::~CompilerDriver_GreenHillsWiiU() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_GreenHillsWiiU::ProcessArg_PreprocessorOnly( const AString & token,
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
/*virtual*/ void CompilerDriver_GreenHillsWiiU::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += "-E"; // run pre-processor only
}

//------------------------------------------------------------------------------
