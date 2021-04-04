// CompilerDriver_SNC.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_SNC
//------------------------------------------------------------------------------
class CompilerDriver_SNC : public CompilerDriverBase
{
public:
    CompilerDriver_SNC();
    virtual ~CompilerDriver_SNC() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
};

//------------------------------------------------------------------------------
