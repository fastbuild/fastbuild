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

    virtual bool ProcessArg_PreprocessorOnly( const AString & token,
                                              size_t & index,
                                              const AString & nextToken,
                                              Args & outFullArgs ) const override;
    virtual bool ProcessArg_CompilePreprocessed( const AString & token,
                                                 size_t & index,
                                                 const AString & nextToken,
                                                 bool isLocal,
                                                 Args & outFullArgs ) const override;
    virtual bool ProcessArg_Common( const AString & token,
                                    size_t & index,
                                    Args & outFullArgs ) const override;

    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
    virtual void AddAdditionalArgs_Common( bool isLocal,
                                           Args & outFullArgs ) const override;

    // Locally modify args before passing to remote worker
    virtual bool ProcessArg_PreparePreprocessedForRemote( const AString & token,
                                                          size_t & index,
                                                          const AString & nextToken,
                                                          Args & outFullArgs ) const override;

protected:
    // Helpers
    bool ProcessArg_XLanguageOption( const AString & token,
                                     size_t & index,
                                     const AString & nextToken,
                                     Args & outFullArgs ) const;
    bool ProcessArg_DependencyOption( const AString & token,
                                      size_t & index ) const;

    bool m_IsClang = false;
};

//------------------------------------------------------------------------------
