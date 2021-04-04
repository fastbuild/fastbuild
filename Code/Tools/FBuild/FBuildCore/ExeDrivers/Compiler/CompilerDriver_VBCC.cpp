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
                                                                   Args & /*outFullArgs*/ )
{
    if ( StripTokenWithArg( "-o", token, index ) )
    {
        return true;
    }

    return false;
}

// ProcessArg_CompilePreprocessed
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_VBCC::ProcessArg_CompilePreprocessed( const AString & /*token*/,
                                                                      size_t & /*index*/,
                                                                      const AString & /*nextToken*/,
                                                                      Args & /*outFullArgs*/ )
{
    return false;
}

// ProcessArg_Common
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_VBCC::ProcessArg_Common( const AString & token,
                                                         size_t & index,
                                                         Args & outFullArgs )
{
    return CompilerDriverBase::ProcessArg_Common( token, index, outFullArgs );
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_VBCC::AddAdditionalArgs_Preprocessor( Args & outFullArgs )
{
    outFullArgs += "-E"; // run pre-processor only
}

// AddAdditionalArgs_Common
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_VBCC::AddAdditionalArgs_Common( Args & /*outFullArgs*/ )
{
}

//------------------------------------------------------------------------------
