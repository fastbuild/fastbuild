// CompilerDriver_OrbisWavePSSLC.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_OrbisWavePSSLC.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_OrbisWavePSSLC::CompilerDriver_OrbisWavePSSLC() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_OrbisWavePSSLC::~CompilerDriver_OrbisWavePSSLC() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_OrbisWavePSSLC::ProcessArg_PreprocessorOnly( const AString & token,
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

// ProcessArg_CompilePreprocessed
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_OrbisWavePSSLC::ProcessArg_CompilePreprocessed( const AString & token,
                                                                          size_t & index,
                                                                          const AString & /*nextToken*/,
                                                                          bool /*isLocal*/,
                                                                          Args & /*outFullArgs*/ ) const
{
    // Remove forced includes so they aren't forced twice
    if ( StripTokenWithArg( "-include", token, index ) )
    {
        return true;
    }

    return false;
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_OrbisWavePSSLC::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += "-E"; // run pre-processor only
}

//------------------------------------------------------------------------------
