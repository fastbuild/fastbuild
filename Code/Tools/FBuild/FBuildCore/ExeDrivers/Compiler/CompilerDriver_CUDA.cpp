// CompilerDriver_CUDA.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_CUDA.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_CUDA::CompilerDriver_CUDA() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_CUDA::~CompilerDriver_CUDA() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_CUDA::ProcessArg_PreprocessorOnly( const AString & token,
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
/*virtual*/ void CompilerDriver_CUDA::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += "-E"; // run pre-processor only
}

//------------------------------------------------------------------------------
