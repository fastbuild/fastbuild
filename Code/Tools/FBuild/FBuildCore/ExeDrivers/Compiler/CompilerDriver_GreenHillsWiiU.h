// CompilerDriver_GreenHillsWiiU.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_GreenHillsWiiU
//------------------------------------------------------------------------------
class CompilerDriver_GreenHillsWiiU : public CompilerDriverBase
{
public:
    CompilerDriver_GreenHillsWiiU();
    virtual ~CompilerDriver_GreenHillsWiiU() override;

protected:
    virtual bool ProcessArg_PreprocessorOnly( const AString & token, size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
};

//------------------------------------------------------------------------------
