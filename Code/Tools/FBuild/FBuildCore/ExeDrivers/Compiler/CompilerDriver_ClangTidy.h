// CompilerDriver_ClangTidy.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Env/Assert.h"
#include "Core/Process/Process.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_ClangTidy
//------------------------------------------------------------------------------
class CompilerDriver_ClangTidy : public CompilerDriverBase
{
public:
    CompilerDriver_ClangTidy();
    virtual ~CompilerDriver_ClangTidy() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;
    virtual bool ProcessArg_Common( const AString & token,
                                    size_t & index,
                                    Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
    virtual void AddAdditionalArgs_Common( bool isLocal,
                                           Args & outFullArgs ) const override;
    virtual void AddPreliminaryArgs( bool isLocal,
                                     Args& outFullArgs) const override;
};

//------------------------------------------------------------------------------
