// CompilerDriver_GCCClang.h
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

// CompilerDriver_GCCClang
//------------------------------------------------------------------------------
class CompilerDriver_GCCClang : public CompilerDriverBase
{
public:
    explicit CompilerDriver_GCCClang( bool isClang );
    virtual ~CompilerDriver_GCCClang() override;

    virtual bool ProcessArg_PreprocessorOnly( const AString & token, size_t & index, const AString & nextToken, Args & outFullArgs ) override;
    virtual bool ProcessArg_CompilePreprocessed( const AString & token, size_t & index, const AString & nextToken, Args & outFullArgs ) override;
    virtual bool ProcessArg_Common( const AString & token, size_t & index, Args & outFullArgs ) override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) override;
    virtual void AddAdditionalArgs_Common( Args & outFullArgs ) override;

protected:
    bool m_IsClang = false;
};

//------------------------------------------------------------------------------
