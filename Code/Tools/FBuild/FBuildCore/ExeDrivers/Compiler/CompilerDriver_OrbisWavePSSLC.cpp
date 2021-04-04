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
                                                                             Args & /*outFullArgs*/ )
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
                                                                          Args & /*outFullArgs*/ )
{
    // Remove forced includes so they aren't forced twice
    if ( StripTokenWithArg( "-include", token, index ) )
    {
        return true;
    }

    return false;
}

// ProcessArg_Common
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_OrbisWavePSSLC::ProcessArg_Common( const AString & token,
                                                                   size_t & index,
                                                                   Args & outFullArgs )
{
    return CompilerDriverBase::ProcessArg_Common( token, index, outFullArgs );
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_OrbisWavePSSLC::AddAdditionalArgs_Preprocessor( Args & outFullArgs )
{
    outFullArgs += "-E"; // run pre-processor only
}

// AddAdditionalArgs_Common
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_OrbisWavePSSLC::AddAdditionalArgs_Common( Args & /*outFullArgs*/ )
{
}

//------------------------------------------------------------------------------
