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

    virtual bool ProcessArg_PreprocessorOnly( const AString & token, size_t & index, const AString & nextToken, Args & outFullArgs ) override;
    virtual bool ProcessArg_CompilePreprocessed( const AString & token, size_t & index, const AString & nextToken, Args & outFullArgs ) override;
    virtual bool ProcessArg_Common( const AString & token, size_t & index, Args & outFullArgs ) override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) override;
    virtual void AddAdditionalArgs_Common( Args & outFullArgs ) override;
};

//------------------------------------------------------------------------------
