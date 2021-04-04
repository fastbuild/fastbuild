// FunctionObjectList
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionObjectList.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/AliasNode.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// Core
#include "Core/FileIO/PathUtils.h"

// system
#include <string.h> // for strlen - TODO:C Remove

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionObjectList::FunctionObjectList()
: Function( "ObjectList" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionObjectList::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionObjectList::NeedsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionObjectList::CreateNode() const
{
    return FNEW( ObjectListNode );
}

// CheckCompilerOptions
//------------------------------------------------------------------------------
bool FunctionObjectList::CheckCompilerOptions( const BFFToken * iter, const AString & compilerOptions, const ObjectNode::CompilerFlags objFlags ) const
{
    bool hasInputToken = false;
    bool hasOutputToken = false;
    bool hasCompileToken = false;

    Array< AString > tokens;
    compilerOptions.Tokenize( tokens );
    for ( const AString & token : tokens )
    {
        if ( token.Find( "%1" ) )
        {
            hasInputToken = true;
        }
        else if ( token.Find( "%2" ) )
        {
            hasOutputToken = true;
        }
        else
        {
            if ( objFlags.IsMSVC() || objFlags.IsClangCl() )
            {
                if ( ( token == "/c" ) || ( token == "-c" ) )
                {
                    hasCompileToken = true;
                }
            }
            else
            {
                if ( token == "-c" )
                {
                    hasCompileToken = true;
                }
            }
        }
    }

    if ( hasInputToken == false )
    {
        Error::Error_1106_MissingRequiredToken( iter, this, ".CompilerOptions", "%1" );
        return false;
    }
    if ( hasOutputToken == false )
    {
        Error::Error_1106_MissingRequiredToken( iter, this, ".CompilerOptions", "%2" );
        return false;
    }

    // check /c or -c
    if ( objFlags.IsMSVC() || objFlags.IsClangCl() )
    {
        if ( hasCompileToken == false )
        {
            Error::Error_1106_MissingRequiredToken( iter, this, ".CompilerOptions", "/c or -c" );
            return false;
        }
    }
    else if ( objFlags.IsSNC() || objFlags.IsGCC() || objFlags.IsVBCC() )
    {
        if ( hasCompileToken == false )
        {
            Error::Error_1106_MissingRequiredToken( iter, this, ".CompilerOptions", "-c" );
            return false;
        }
    }

    return true;
}

// CheckMSVCPCHFlags_Create
//------------------------------------------------------------------------------
bool FunctionObjectList::CheckMSVCPCHFlags_Create( const BFFToken * iter,
                                                   const AString & pchOptions,
                                                   const AString & pchOutputFile,
                                                   const char * compilerOutputExtension,
                                                   AString & pchObjectName ) const
{
    // sanity check arguments
    bool foundYcInPCHOptions = false;
    bool foundFpInPCHOptions = false;

    // Find /Fo option to obtain pch object file name
    Array< AString > pchTokens;
    pchOptions.Tokenize( pchTokens );
    for ( const AString & token : pchTokens )
    {
        if ( ObjectNode::IsStartOfCompilerArg_MSVC( token, "Fo" ) )
        {
            // Extract filename (and remove quotes if found)
            pchObjectName = token.Get() + 3;
            pchObjectName.Trim( pchObjectName.BeginsWith( '"' ) ? 1u : 0u, pchObjectName.EndsWith( '"' ) ? 1u : 0u );

            // Auto-generate name?
            if ( pchObjectName == "%3" )
            {
                // example 'PrecompiledHeader.pch' to 'PrecompiledHeader.pch.obj'
                pchObjectName = pchOutputFile;
                pchObjectName += compilerOutputExtension;
            }
        }
        else if ( ObjectNode::IsStartOfCompilerArg_MSVC( token, "Yc" ) )
        {
            foundYcInPCHOptions = true;
        }
        else if ( ObjectNode::IsStartOfCompilerArg_MSVC( token, "Fp" ) )
        {
            foundFpInPCHOptions = true;
        }
    }

    // PCH must have "Create PCH" (e.g. /Yc"PrecompiledHeader.h")
    if ( foundYcInPCHOptions == false )
    {
        Error::Error_1302_MissingPCHCompilerOption( iter, this, "Yc", "PCHOptions" );
        return false;
    }
    // PCH must have "Precompiled Header to Use" (e.g. /Fp"PrecompiledHeader.pch")
    if ( foundFpInPCHOptions == false )
    {
        Error::Error_1302_MissingPCHCompilerOption( iter, this, "Fp", "PCHOptions" );
        return false;
    }
    // PCH must have object output option (e.g. /Fo"PrecompiledHeader.obj")
    if ( pchObjectName.IsEmpty() )
    {
        Error::Error_1302_MissingPCHCompilerOption( iter, this, "Fo", "PCHOptions" );
        return false;
    }

    return true;
}


// CheckMSVCPCHFlags_Use
//------------------------------------------------------------------------------
bool FunctionObjectList::CheckMSVCPCHFlags_Use( const BFFToken * iter,
                                                const AString & compilerOptions,
                                                ObjectNode::CompilerFlags objFlags ) const
{
    // Check Compiler Options
    bool foundYuInCompilerOptions = false;
    bool foundFpInCompilerOptions = false;
    Array< AString > compilerTokens;
    compilerOptions.Tokenize( compilerTokens );
    for ( const AString & token : compilerTokens )
    {
        if ( ObjectNode::IsStartOfCompilerArg_MSVC( token, "Yu" ) )
        {
            foundYuInCompilerOptions = true;
        }
        else if ( ObjectNode::IsStartOfCompilerArg_MSVC( token, "Fp" ) )
        {
            foundFpInCompilerOptions = true;
        }
    }

    // Object using the PCH must have "Use PCH" option (e.g. /Yu"PrecompiledHeader.h")
    if ( foundYuInCompilerOptions == false )
    {
        Error::Error_1302_MissingPCHCompilerOption( iter, this, "Yu", "CompilerOptions" );
        return false;
    }
    // Object using the PCH must have "Precompiled header to use" (e.g. /Fp"PrecompiledHeader.pch")
    if ( foundFpInCompilerOptions == false )
    {
        Error::Error_1302_MissingPCHCompilerOption( iter, this, "Fp", "CompilerOptions" );
        return false;
    }

    if ( objFlags.IsCreatingPCH() )
    {
        // must not specify use of precompiled header (must use the PCH specific options)
        Error::Error_1303_PCHCreateOptionOnlyAllowedOnPCH( iter, this, "Yc", "CompilerOptions" );
        return false;
    }

    return true;
}

// GetExtraOutputPaths
//------------------------------------------------------------------------------
void FunctionObjectList::GetExtraOutputPaths( const AString & args, AString & pdbPath, AString & asmPath )
{
    // split to individual tokens
    Array< AString > tokens;
    args.Tokenize( tokens );

    const AString * const end = tokens.End();
    for ( const AString * it = tokens.Begin(); it != end; ++it )
    {
        if ( ObjectNode::IsStartOfCompilerArg_MSVC( *it, "Fd" ) )
        {
            GetExtraOutputPath( it, end, "Fd", pdbPath );
            continue;
        }

        if ( ObjectNode::IsStartOfCompilerArg_MSVC( *it, "Fa" ) )
        {
            GetExtraOutputPath( it, end, "Fa", asmPath );
            continue;
        }
    }
}

// GetExtraOutputPath
//------------------------------------------------------------------------------
/*static*/ void FunctionObjectList::GetExtraOutputPath( const AString * it, const AString * end, const char * option, AString & path )
{
    const char * bodyStart = it->Get() + strlen( option ) + 1; // +1 for - or /
    const char * bodyEnd = it->GetEnd();

    // if token is exactly matched then value is next token
    if ( bodyStart == bodyEnd )
    {
        ++it;
        // handle missing next value
        if ( it == end )
        {
            return; // we just pretend it doesn't exist and let the compiler complain
        }

        bodyStart = it->Get();
        bodyEnd = it->GetEnd();
    }

    // Strip quotes
    Args::StripQuotes( bodyStart, bodyEnd, path );

    // Normalize path
    if ( PathUtils::IsFolderPath( path ) )
    {
        PathUtils::FixupFolderPath( path );
    }
    else
    {
        PathUtils::FixupFilePath( path );

        // truncate to just the path
        const char * lastSlash = path.FindLast( NATIVE_SLASH );
        lastSlash  = lastSlash ? lastSlash : path.Get(); // no slash, means it's just a filename
        path.SetLength( uint32_t( lastSlash - path.Get() ) );
    }
}

//------------------------------------------------------------------------------
