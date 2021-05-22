// CompilerDriver_QtRCC.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_QtRCC
//------------------------------------------------------------------------------
class CompilerDriver_QtRCC : public CompilerDriverBase
{
public:
    CompilerDriver_QtRCC();
    virtual ~CompilerDriver_QtRCC() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
};

//------------------------------------------------------------------------------
