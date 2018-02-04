// FunctionObjectList
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionObjectList::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    ObjectListNode * objectListNode = nodeGraph.CreateObjectListNode( m_AliasForFunction );

    if ( !PopulateProperties( nodeGraph, funcStartIter, objectListNode ) )
    {
        return false;
    }

    if ( !objectListNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    return true;
}

// CheckCompilerOptions
//------------------------------------------------------------------------------
bool FunctionObjectList::CheckCompilerOptions( const BFFIterator & iter, const AString & compilerOptions, const uint32_t objFlags ) const
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
            if ( objFlags & ObjectNode::FLAG_MSVC )
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
    if ( objFlags & ObjectNode::FLAG_MSVC )
    {
        if ( hasCompileToken == false )
        {
            Error::Error_1106_MissingRequiredToken( iter, this, ".CompilerOptions", "/c or -c" );
            return false;
        }
    }
    else if ( objFlags & ( ObjectNode::FLAG_SNC | ObjectNode::FLAG_GCC | ObjectNode::FLAG_CLANG | ObjectNode::FLAG_VBCC ) )
    {
        if ( hasCompileToken == false )
        {
            Error::Error_1106_MissingRequiredToken( iter, this, ".CompilerOptions", "-c" );
            return false;
        }
    }

    return true;
}

// GetCompilerNode
//------------------------------------------------------------------------------
bool FunctionObjectList::GetCompilerNode( NodeGraph & nodeGraph, const BFFIterator & iter, const AString & compiler, CompilerNode * & compilerNode ) const
{
    Node * cn = nodeGraph.FindNode( compiler );
    compilerNode = nullptr;
    if ( cn != nullptr )
    {
        if ( cn->GetType() == Node::ALIAS_NODE )
        {
            AliasNode * an = cn->CastTo< AliasNode >();
            cn = an->GetAliasedNodes()[ 0 ].GetNode();
        }
        if ( cn->GetType() != Node::COMPILER_NODE )
        {
            Error::Error_1102_UnexpectedType( iter, this, "Compiler", cn->GetName(), cn->GetType(), Node::COMPILER_NODE );
            return false;
        }
        compilerNode = cn->CastTo< CompilerNode >();
    }
    else
    {
        // create a compiler node - don't allow distribution
        // (only explicitly defined compiler nodes can be distributed)
        // set the default executable path to be the compiler exe directory
        AStackString<> compilerClean;
        NodeGraph::CleanPath( compiler, compilerClean );
        compilerNode = nodeGraph.CreateCompilerNode( compilerClean );
        VERIFY( compilerNode->GetReflectionInfoV()->SetProperty( compilerNode, "AllowDistribution", false ) );
        const char * lastSlash = compilerClean.FindLast( NATIVE_SLASH );
        if ( lastSlash )
        {
            AStackString<> executableRootPath( compilerClean.Get(), lastSlash + 1 );
            VERIFY( compilerNode->GetReflectionInfoV()->SetProperty( compilerNode, "ExecutableRootPath", executableRootPath ) );
        }
        if ( !compilerNode->Initialize( nodeGraph, iter, nullptr ) )
        {
            return false; // Initialize will have emitted an error
        }
    }

    return true;
}

// CheckMSVCPCHFlags
//------------------------------------------------------------------------------
bool FunctionObjectList::CheckMSVCPCHFlags( const BFFIterator & iter,
                                            const AString & compilerOptions,
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
            pchObjectName.Trim( pchObjectName.BeginsWith( '"' ) ? 1 : 0, pchObjectName.EndsWith( '"' ) ? 1 : 0 );

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

    return true;
}

// GetExtraOutputPaths
//------------------------------------------------------------------------------
void FunctionObjectList::GetExtraOutputPaths( const AString & args, AString & pdbPath, AString & asmPath ) const
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
void FunctionObjectList::GetExtraOutputPath( const AString * it, const AString * end, const char * option, AString & path ) const
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

    // If it's not already a path (i.e. includes filename.ext) then
    // truncate to just the path
    const char * lastSlash = path.FindLast( '\\' );
    lastSlash = lastSlash ? lastSlash : path.FindLast( '/' );
    lastSlash  = lastSlash ? lastSlash : path.Get(); // no slash, means it's just a filename
    if ( lastSlash != ( path.GetEnd() - 1 ) )
    {
        path.SetLength( uint32_t(lastSlash - path.Get()) );
    }
}

//------------------------------------------------------------------------------
