// CompilerDriverBase.h 
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Env/Types.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Args;
class Job;
class ObjectNode;

// CompilerDriverBase
//------------------------------------------------------------------------------
class CompilerDriverBase
{
public:
    CompilerDriverBase();
    virtual ~CompilerDriverBase();

    void Init( const ObjectNode * objectNode,
               bool isLocal,
               const AString & remoteSourceRoot );

    void SetForceColoredDiagnostics( bool forceColoredDiagnostics ) { m_ForceColoredDiagnostics = forceColoredDiagnostics; }
    void SetUseSourceMapping( const AString & sourceMapping ) { m_SourceMapping = sourceMapping; }
    void SetRelativeBasePath( const AString & relativeBasePath ) { m_RelativeBasePath = relativeBasePath; }
    void SetOverrideSourceFile( const AString & overrideSourceFile ) { m_OverrideSourceFile= overrideSourceFile; }

    // Manipulate args if needed for various compilation modes
    virtual bool ProcessArg_PreprocessorOnly( const AString & token, size_t & index, const AString & nextToken, Args & outFullArgs );
    virtual bool ProcessArg_CompilePreprocessed( const AString & token, size_t & index, const AString & nextToken, Args & outFullArgs );
    virtual bool ProcessArg_Common( const AString & token, size_t & index, Args & outFullArgs );

    // Add additional args
    virtual void AddAdditionalArgs_Preprocessor( Args & outFullArgs );
    virtual void AddAdditionalArgs_Common( Args & outFullArgs );

protected:
    static bool StripTokenWithArg( const char * tokenToCheckFor, const AString & token, size_t & index );
    static bool StripToken( const char * tokenToCheckFor, const AString & token, bool allowStartsWith = false );

    const ObjectNode *  m_ObjectNode                = nullptr;
    bool                m_IsLocal                   = true;
    bool                m_ForceColoredDiagnostics   = false;
    AString             m_SourceMapping;
    AString             m_RelativeBasePath;
    AString             m_OverrideSourceFile;
    AString             m_RemoteSourceRoot;
};

//------------------------------------------------------------------------------
