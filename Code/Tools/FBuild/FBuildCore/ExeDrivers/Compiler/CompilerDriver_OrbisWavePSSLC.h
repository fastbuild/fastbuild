// CompilerDriver_OrbisWavePSSLC.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_OrbisWavePSSLC
//------------------------------------------------------------------------------
class CompilerDriver_OrbisWavePSSLC : public CompilerDriverBase
{
public:
    CompilerDriver_OrbisWavePSSLC();
    virtual ~CompilerDriver_OrbisWavePSSLC() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;
    virtual bool ProcessArg_CompilePreprocessed( const AString & token,
                                                 size_t & index,
                                                 const AString & nextToken,
                                                 bool isLocal,
                                                 Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
};

//------------------------------------------------------------------------------
