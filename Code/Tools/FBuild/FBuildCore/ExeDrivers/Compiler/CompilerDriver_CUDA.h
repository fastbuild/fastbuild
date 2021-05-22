// CompilerDriver_CUDA.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_CUDA
//------------------------------------------------------------------------------
class CompilerDriver_CUDA : public CompilerDriverBase
{
public:
    CompilerDriver_CUDA();
    virtual ~CompilerDriver_CUDA() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
};

//------------------------------------------------------------------------------
