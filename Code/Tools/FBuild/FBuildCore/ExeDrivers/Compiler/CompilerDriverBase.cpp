// CompilerDriverBase.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriverBase.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h" // TODO: Can we get rid of this?
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h" // TODO: Remove?

// Core
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriverBase::CompilerDriverBase() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriverBase::~CompilerDriverBase() = default;

// Init
//------------------------------------------------------------------------------
void CompilerDriverBase::Init( const ObjectNode * objectNode,
                               const AString & remoteSourceRoot )
{
    m_ObjectNode= objectNode;
    m_RemoteSourceRoot = remoteSourceRoot;
}

//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriverBase::ProcessArg_PreprocessorOnly( const AString & /*token*/,
                                                                  size_t & /*index*/,
                                                                  const AString & /*nextToken*/,
                                                                  Args & /*outFullArgs*/ ) const
{
    return false;
}

// ProcessArg_CompilePreprocessed
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriverBase::ProcessArg_CompilePreprocessed( const AString & /*token*/,
                                                                     size_t & /*index*/,
                                                                     const AString & /*nextToken*/,
                                                                     bool /*isLocal*/,
                                                                     Args & /*outFullArgs*/ ) const
{
    return false;
}

// ProcessArg_Common
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriverBase::ProcessArg_Common( const AString & /*token*/,
                                                        size_t & /*index*/,
                                                        Args & /*outFullArgs*/ ) const
{
    return false;
}

// ProcessArg_BuildTimeSubstitution
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriverBase::ProcessArg_BuildTimeSubstitution( const AString & token,
                                                                       size_t & /*index*/,
                                                                       Args & outFullArgs ) const
{
    // %1 -> InputFile
    {
        const char * const found = token.Find( "%1" );
        if ( found )
        {
            outFullArgs += AStackString<>( token.Get(), found );
            if ( m_OverrideSourceFile.IsEmpty() )
            {
                if ( m_RelativeBasePath.IsEmpty() == false )
                {
                    AStackString<> relativeFileName;
                    PathUtils::GetRelativePath( m_RelativeBasePath, m_ObjectNode->GetSourceFile()->GetName(), relativeFileName );
                    outFullArgs += relativeFileName;
                }
                else
                {
                    outFullArgs += m_ObjectNode->GetSourceFile()->GetName();
                }
            }
            else
            {
                outFullArgs += m_OverrideSourceFile;
            }
            outFullArgs += AStackString<>( found + 2, token.GetEnd() );
            outFullArgs.AddDelimiter();
            return true;
        }
    }

    // %2 -> OutputFile
    {
        const char * const found = token.Find( "%2" );
        if ( found )
        {
            outFullArgs += AStackString<>( token.Get(), found );
            if ( m_RelativeBasePath.IsEmpty() == false )
            {
                AStackString<> relativeFileName;
                PathUtils::GetRelativePath( m_RelativeBasePath, m_ObjectNode->GetName(), relativeFileName );
                outFullArgs += relativeFileName;
            }
            else
            {
                outFullArgs += m_ObjectNode->GetName();
            }
            outFullArgs += AStackString<>( found + 2, token.GetEnd() );
            outFullArgs.AddDelimiter();
            return true;
        }
    }

    return false;
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriverBase::AddAdditionalArgs_Preprocessor( Args & /*outFullArgs*/ ) const
{
}

// AddAdditionalArgs_Common
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriverBase::AddAdditionalArgs_Common( bool /*isLocal*/,
                                                               Args & /*outFullArgs*/ ) const
{
}

// StripTokenWithArg
//------------------------------------------------------------------------------
/*static*/ bool CompilerDriverBase::StripTokenWithArg( const char * tokenToCheckFor,
                                                       const AString & token,
                                                       size_t & index )
{
    if ( token.BeginsWith( tokenToCheckFor ) )
    {
        if ( token == tokenToCheckFor )
        {
            ++index; // skip additional next token
        }
        return true; // found
    }
    if ( token.BeginsWith( '"' ) && token.EndsWith( '"' ) && ( token.GetLength() > 2 ) )
    {
        const AStackString<> unquoted( ( token.Get() + 1 ), ( token.GetEnd() - 1 ) );
        return StripTokenWithArg( tokenToCheckFor, unquoted, index );
    }
    return false; // not found
}

// StripToken
//------------------------------------------------------------------------------
/*static*/ bool CompilerDriverBase::StripToken( const char * tokenToCheckFor,
                                                const AString & token,
                                                bool allowStartsWith )
{
    if ( allowStartsWith )
    {
        return token.BeginsWith( tokenToCheckFor );
    }
    else
    {
        return ( token == tokenToCheckFor );
    }
}

// ProcessArg_PreparePreprocessedForRemote
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriverBase::ProcessArg_PreparePreprocessedForRemote( const AString & /*token*/,
                                                                              size_t & /*index*/,
                                                                              const AString & /*nextToken*/,
                                                                              Args & /*outFullArgs*/ ) const
{
    return false;
}

// AddAdditionalArgs_PreparePreprocessedForRemote
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriverBase::AddAdditionalArgs_PreparePreprocessedForRemote( Args & /*outFullArgs*/ )
{
}

//------------------------------------------------------------------------------
