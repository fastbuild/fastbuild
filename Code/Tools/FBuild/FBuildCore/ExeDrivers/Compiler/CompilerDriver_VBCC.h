// CompilerDriver_VBCC.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_VBCC
//------------------------------------------------------------------------------
class CompilerDriver_VBCC : public CompilerDriverBase
{
public:
    CompilerDriver_VBCC();
    virtual ~CompilerDriver_VBCC() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
};

//------------------------------------------------------------------------------
