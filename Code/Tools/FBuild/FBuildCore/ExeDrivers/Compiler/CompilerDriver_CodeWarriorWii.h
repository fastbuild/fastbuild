// CompilerDriver_CodeWarriorWii.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_CodeWarriorWii
//------------------------------------------------------------------------------
class CompilerDriver_CodeWarriorWii : public CompilerDriverBase
{
public:
    CompilerDriver_CodeWarriorWii();
    virtual ~CompilerDriver_CodeWarriorWii() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
};

//------------------------------------------------------------------------------
