// CompilerDriver_ClangTidy.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_ClangTidy.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_ClangTidy::CompilerDriver_ClangTidy() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_ClangTidy::~CompilerDriver_ClangTidy() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_ClangTidy::ProcessArg_PreprocessorOnly( const AString & /*token*/,
                                                                       size_t & /*index*/,
                                                                       const AString & /*nextToken*/,
                                                                       Args & /*outFullArgs*/ ) const
{
    ASSERT(false); // ClangTidy can't be used as a preprocessor
    return false;
}

// ProcessArg_Common
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_ClangTidy::ProcessArg_Common( const AString & token,
                                                             size_t & index,
                                                             Args & outFullArgs ) const
{
    if ( StripToken("--config-file=", token, true) )
    {
        return true;
    }

    // Strip -fdiagnostics-color options because we are going to override them
    if ( m_ForceColoredDiagnostics )
    {
        if ( StripToken( "-fdiagnostics-color", token, true ) ||
             StripToken( "-fno-diagnostics-color", token ) )
        {
            return true;
        }
    }

    return CompilerDriverBase::ProcessArg_Common( token, index, outFullArgs );
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_ClangTidy::AddAdditionalArgs_Preprocessor( Args & /*outFullArgs*/) const
{
    ASSERT(false); // ClangTidy can't be used as a preprocessor
}

// AddAdditionalArgs_Common
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_ClangTidy::AddAdditionalArgs_Common( bool /*isLocal*/,
                                                                    Args & outFullArgs ) const
{
    if ( m_ForceColoredDiagnostics )
    {
        outFullArgs += " -fdiagnostics-color=always";
    }
}

// AddPreliminaryArgs
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_ClangTidy::AddPreliminaryArgs( bool /*isLocal*/,
                                                         Args & outFullArgs ) const
{
    // clang-tidy accepts two sets of arguments, its linter arguments and optional
    // compilation arguments forwarded to the underlying clang instance
    // typical usage is:
    // $ clang-tidy [linter_arguments...] -- [clang_arguments...]
    // --config-file being a linter argument we rely on, we make sure it is always
    // passed before clang arguments

    outFullArgs += "--config-file=";
    outFullArgs += m_OverrideSourceFile;
    outFullArgs += ".config.yaml";
    outFullArgs.AddDelimiter();
}

//------------------------------------------------------------------------------
