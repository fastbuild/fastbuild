// CompilerDriver_CL.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_CL.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// Core
#include "Core/FileIO/PathUtils.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_CL::CompilerDriver_CL( bool isClangCL )
    : m_IsClangCL( isClangCL )
{}

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_CL::~CompilerDriver_CL() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_CL::ProcessArg_PreprocessorOnly( const AString & token,
                                                                 size_t & index,
                                                                 const AString & /*nextToken*/,
                                                                 Args & outFullArgs ) const
{
    // Strip /ZW
    if ( StripToken_MSVC( "ZW", token ) )
    {
        outFullArgs += "/D__cplusplus_winrt ";
        return true;
    }

    // Strip /Yc (pch creation)
    if ( StripTokenWithArg_MSVC( "Yc", token, index ) )
    {
        return true;
    }

    // Remove static analyzer from clang preprocessor
    if ( m_IsClangCL )
    {
        if ( StripToken( "--analyze", token ) ||
             StripTokenWithArg( "-Xanalyzer", token, index ) ||
             StripTokenWithArg( "-analyzer-output", token, index ) ||
             StripTokenWithArg( "-analyzer-config", token, index ) ||
             StripTokenWithArg( "-analyzer-checker", token, index ) )
        {
            return true;
        }
    }

    return false;
}

// ProcessArg_CompilePreprocessed
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_CL::ProcessArg_CompilePreprocessed( const AString & token,
                                                                    size_t & index,
                                                                    const AString & /*nextToken*/,
                                                                    bool isLocal,
                                                                    Args & outFullArgs ) const
{
    // Can't use the precompiled header when compiling the preprocessed output
    // as this would prevent cacheing.
    if ( StripTokenWithArg_MSVC( "Yu", token, index ) )
    {
        return true;
    }
    if ( StripTokenWithArg_MSVC( "Fp", token, index ) )
    {
        return true;
    }

    // Remote compilation writes to a temp pdb
    if ( isLocal == false )
    {
        if ( StripTokenWithArg_MSVC( "Fd", token, index ) )
        {
            return true;
        }
    }

    // NOTE: Leave /I includes for compatibility with Recode
    // (unlike Clang, MSVC is ok with leaving the /I when compiling preprocessed code)

    // To prevent D8049 "command line is too long to fit in debug record"
    // we expand relative includes as they would be on the host (so the remote machine's
    // working dir is not used, which might be longer, causing this overflow an internal
    // limit of cl.exe)
    if ( ( isLocal == false ) && IsStartOfCompilerArg_MSVC( token, "I" ) )
    {
        // Get include path part
        const char * start = token.Get() + 2; // Skip /I or -I
        const char * end = token.GetEnd();

        // strip quotes if present
        if ( *start == '"' )
        {
            ++start;
        }
        if ( end[ -1 ] == '"' )
        {
            --end;
        }
        AStackString<> includePath( start, end );
        const bool isFullPath = PathUtils::IsFullPath( includePath );

        // Replace relative paths and leave full paths alone
        if ( isFullPath == false )
        {
            // Remove relative include
            StripTokenWithArg_MSVC( "I", token, index );

            // Add full path include
            outFullArgs.Append( token.Get(), (size_t)( start - token.Get() ) );
            outFullArgs += m_RemoteSourceRoot;
            outFullArgs += '\\';
            outFullArgs += includePath;
            outFullArgs.Append( end, (size_t)( token.GetEnd() - end ) );
            outFullArgs.AddDelimiter();
            return true;
        }
    }

    // Strip "Force Includes" statements (as they are merged in already during preprocessing)
    if ( StripTokenWithArg_MSVC( "FI", token, index ) )
    {
        return true;
    }

    return false;
}

// ProcessArg_Common
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_CL::ProcessArg_Common( const AString & token,
                                                       size_t & /*index*/,
                                                       Args & outFullArgs ) const
{
    // FASTBuild handles the multiprocessor scheduling
    if ( StripToken_MSVC( "MP", token, true ) ) // true = strip '/MP' and starts with '/MP'
    {
        return true;
    }

    // "Minimal Rebuild" is not compatible with FASTBuild
    if ( StripToken_MSVC( "Gm", token ) )
    {
        return true;
    }

    // cl.exe treats \" as an escaped quote
    // It's a common user error to terminate things (like include paths) with a quote
    // this way, messing up the rest of the args and causing bizarre failures.
    // Since " is not a valid character in a path, just strip the escape char
    //
    // Is this invalid?
    //  bad: /I"directory\"  - TODO:B Handle other args with this problem
    //  ok : /I\"directory\"
    //  ok : /I"directory"
    if ( ObjectNode::IsStartOfCompilerArg_MSVC( token, "I\"" ) && token.EndsWith( "\\\"" ) )
    {
        outFullArgs.Append( token.Get(), token.GetLength() - 2 );
        outFullArgs += '"';
        outFullArgs.AddDelimiter();
        return true;
    }

    return false;
}

// ProcessArg_BuildTimeSubstitution
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_CL::ProcessArg_BuildTimeSubstitution( const AString & token,
                                                                      size_t & index,
                                                                      Args & outFullArgs ) const
{
    // %3 -> PrecompiledHeader Obj
    {
        const char * const found = token.Find( "%3" );
        if ( found )
        {
            // handle /Option:%3 -> /Option:A
            const AString & pchObjectFileName = m_ObjectNode->GetPCHObjectName();
            outFullArgs += AStackString<>( token.Get(), found );
            ASSERT( pchObjectFileName.IsEmpty() == false ); // Should have been populated
            outFullArgs += pchObjectFileName;
            outFullArgs += AStackString<>( found + 2, token.GetEnd() );
            outFullArgs.AddDelimiter();
            return true;
        }
    }

    // %4 -> CompilerForceUsing list
    {
        const char * const found = token.Find( "%4" );
        if ( found )
        {
            const AStackString<> pre( token.Get(), found );
            const AStackString<> post( found + 2, token.GetEnd() );
            m_ObjectNode->ExpandCompilerForceUsing( outFullArgs, pre, post );
            outFullArgs.AddDelimiter();
            return true;
        }
    }

    return CompilerDriverBase::ProcessArg_BuildTimeSubstitution( token, index, outFullArgs );
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_CL::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    // This attempt to define the missing _PREFAST_ macro results in strange
    // inconsistencies when compiling with /analyze
    //if ( GetFlag( FLAG_STATIC_ANALYSIS_MSVC ) )
    //{
    //    // /E disables the /D_PREFAST_ define when used with /analyze
    //    // but we want SAL annotations to still be applied
    //    fullArgs += "/D_PREFAST_=1"; // NOTE: Must be before /E option!
    //    fullArgs.AddDelimiter();
    //}

    outFullArgs += "/E"; // run pre-processor only

    // Ensure unused defines declared in the PCH but not used
    // in the PCH are accounted for (See TestPrecompiledHeaders/CacheUniqueness)
    if ( m_ObjectNode->IsCreatingPCH() )
    {
        outFullArgs += " /d1PP"; // Must be after /E
    }
}

// AddAdditionalArgs_Common
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_CL::AddAdditionalArgs_Common( bool isLocal, Args & outFullArgs ) const
{
    // Remote compilation writes to a temp pdb
    if ( ( isLocal == false ) &&
         ( m_ObjectNode->IsUsingPDB() ) )
    {
        AStackString<> pdbName;
        m_ObjectNode->GetPDBName( pdbName );
        outFullArgs += AStackString<>().Format( " /Fd\"%s\"", pdbName.Get() );
    }

    // Add args for source mapping
    if ( m_IsClangCL )
    {
        if ( ( m_SourceMapping.IsEmpty() == false ) && isLocal )
        {
            const AString& workingDir = FBuild::Get().GetOptions().GetWorkingDir();
            AStackString<> tmp;
            // Using -ffile-prefix-map would be better since that would change not only the file paths in
            // the DWARF debugging information but also in the __FILE__ and related predefined macros, but
            // -ffile-prefix-map is only supported starting with GCC 8 and Clang 10. The -fdebug-prefix-map
            // option is available starting with Clang 3.8 and all modern GCC versions.
            tmp = " -Xclang "; // When clang is operating in "CL mode", it seems to need the -Xclang prefix for the command
            tmp.AppendFormat(" \"-fdebug-prefix-map=%s=%s\"", workingDir.Get(), m_SourceMapping.Get());
            outFullArgs += tmp;
        }
    }
}

// IsCompilerArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool CompilerDriver_CL::IsCompilerArg_MSVC( const AString & token, const char * arg )
{
    ASSERT( token.IsEmpty() == false );

    // MSVC Compiler args can start with - or /
    if ( ( token[0] != '/' ) && ( token[0] != '-' ) )
    {
        return false;
    }

    // Length check to early out
    const size_t argLen = AString::StrLen( arg );
    if ( ( token.GetLength() - 1 ) != argLen )
    {
        return false; // token is too short or too long
    }

    // MSVC Compiler args are case-sensitive
    return token.EndsWith( arg );
}

// IsStartOfCompilerArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool CompilerDriver_CL::IsStartOfCompilerArg_MSVC( const AString & token, const char * arg )
{
    ASSERT( token.IsEmpty() == false );

    // MSVC Compiler args can start with - or /
    if ( ( token[0] != '/' ) && ( token[0] != '-' ) )
    {
        return false;
    }

    // Length check to early out
    const size_t argLen = AString::StrLen( arg );
    if ( ( token.GetLength() - 1 ) < argLen )
    {
        return false; // token is too short
    }

    // MSVC Compiler args are case-sensitive
    return ( AString::StrNCmp( token.Get() + 1, arg, argLen ) == 0 );
}


// StripTokenWithArg_MSVC
//------------------------------------------------------------------------------
/*static*/ bool CompilerDriver_CL::StripTokenWithArg_MSVC( const char * tokenToCheckFor, const AString & token, size_t & index )
{
    if ( IsStartOfCompilerArg_MSVC( token, tokenToCheckFor ) )
    {
        if ( IsCompilerArg_MSVC( token, tokenToCheckFor ) )
        {
            ++index; // skip additional next token
        }
        return true; // found
    }
    return false; // not found
}

// StripToken_MSVC
//------------------------------------------------------------------------------
/*static*/ bool CompilerDriver_CL::StripToken_MSVC( const char * tokenToCheckFor, const AString & token, bool allowStartsWith )
{
    if ( allowStartsWith )
    {
        return IsStartOfCompilerArg_MSVC( token, tokenToCheckFor );
    }
    else
    {
        return IsCompilerArg_MSVC( token, tokenToCheckFor );
    }
}

//------------------------------------------------------------------------------
