// CompilerDriver_CL.h 
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// Forward Declarations
//------------------------------------------------------------------------------

// CompilerDriver_CL
//------------------------------------------------------------------------------
class CompilerDriver_CL : public CompilerDriverBase
{
public:
    explicit CompilerDriver_CL( bool isClangCL );
    virtual ~CompilerDriver_CL() override;

protected:
    // Manipulate args if needed for various compilation modes
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

    // Inject build-time substitutions (%1 etc)
    virtual bool ProcessArg_BuildTimeSubstitution( const AString & token,
                                                   size_t & index,
                                                   Args & outFullArgs ) const override;

    // Add additional args
    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const override;
    virtual void AddAdditionalArgs_Common( bool isLocal,
                                           Args & outFullArgs ) const override;

    static bool IsCompilerArg_MSVC( const AString & token, const char * arg );
    static bool IsStartOfCompilerArg_MSVC( const AString & token, const char * arg );
    static bool StripTokenWithArg_MSVC( const char * tokenToCheckFor,
                                        const AString & token,
                                        size_t & index );
    static bool StripToken_MSVC( const char * tokenToCheckFor,
                                 const AString & token,
                                 bool allowStartsWith = false );

    bool m_IsClangCL = false; // Using clang in CL compatibility mode?
};

//------------------------------------------------------------------------------
