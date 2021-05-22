// CompilerDriver_VBCC.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_VBCC.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_VBCC::CompilerDriver_VBCC() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_VBCC::~CompilerDriver_VBCC() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_VBCC::ProcessArg_PreprocessorOnly( const AString & token,
                                                                   size_t & index,
                                                                   const AString & /*nextToken*/,
                                                                   Args & /*outFullArgs*/ ) const
{
    if ( StripTokenWithArg( "-o", token, index ) )
    {
        return true;
    }

    return false;
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_VBCC::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += "-E"; // run pre-processor only
}

//------------------------------------------------------------------------------
