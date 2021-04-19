// CompilerDriver_GCCClang.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_GCCClang.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_GCCClang::CompilerDriver_GCCClang( bool isClang )
    : m_IsClang( isClang )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_GCCClang::~CompilerDriver_GCCClang() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_GCCClang::ProcessArg_PreprocessorOnly( const AString & token,
                                                                       size_t & index,
                                                                       const AString & /*nextToken*/,
                                                                       Args & /*outFullArgs*/ ) const
{
    // The pch can only be utilized when doing a direct compilation
    //  - Can't be used to generate the preprocessed output
    //  - Can't be used to accelerate compilation of the preprocessed output
    if ( StripTokenWithArg( "-include-pch", token, index ) )
    {
        return true;
    }

    // Remove static analyzer from clang preprocessor
    if ( m_IsClang )
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

    // Remove output arg
    if ( StripTokenWithArg( "-o", token, index ) )
    {
        return true;
    }

    // Remove "compile only" flag
    if ( StripToken( "-c", token ) )
    {
        return true;
    }

    return false;
}

// ProcessArg_CompilePreprocessed
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_GCCClang::ProcessArg_CompilePreprocessed( const AString & token,
                                                                          size_t & index,
                                                                          const AString & nextToken,
                                                                          bool isLocal,
                                                                          Args & outFullArgs ) const
{
    if ( m_IsClang )
    {
        // Clang requires -I options be stripped when compiling preprocessed code
        // (it raises an error if we don't remove these)
        if ( StripTokenWithArg( "-I", token, index ) )
        {
            return true;
        }
    }

    // Remove isysroot, which may not exist on a distributed system, and
    // should only be used for include paths, which have already been
    // processed.
    if ( StripTokenWithArg( "-isysroot", token, index ) )
    {
        return true;
    }

    // Handle -x language option update
    if ( isLocal )
    {
        if ( ProcessArg_XLanguageOption( token, index, nextToken, outFullArgs ) )
        {
            return true;
        }
    }

    // Handle dependency output args (-MD etc)
    if ( isLocal )
    {
        if ( ProcessArg_DependencyOption( token, index ) )
        {
            return true;
        }
    }

    // The pch can only be utilized when doing a direct compilation
    //  - Can't be used to generate the preprocessed output
    //  - Can't be used to accelerate compilation of the preprocessed output
    if ( StripTokenWithArg( "-include-pch", token, index ) )
    {
        return true;
    }

    // Remove forced includes so they aren't forced twice
    if ( StripTokenWithArg( "-include", token, index ) )
    {
        return true;
    }

    return false;
}

// ProcessArg_Common
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_GCCClang::ProcessArg_Common( const AString & token,
                                                             size_t & index,
                                                             Args & outFullArgs ) const
{
    // Strip -fdiagnostics-color options because we are going to override them
    if ( m_ForceColoredDiagnostics )
    {
        if ( StripToken( "-fdiagnostics-color", token, true ) ||
             StripToken( "-fno-diagnostics-color", token ) )
        {
            return true;
        }
    }

    return CompilerDriverBase::ProcessArg_Common( token, index, outFullArgs );
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_GCCClang::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += "-E"; // run pre-processor only

    // Ensure unused defines declared in the PCH but not used
    // in the PCH are accounted for (See TestPrecompiledHeaders/CacheUniqueness)
    if ( m_ObjectNode->IsCreatingPCH() )
    {
        outFullArgs += " -dD";
    }

    if ( m_IsClang )
    {
        const bool clangRewriteIncludes = m_ObjectNode->GetCompiler()->IsClangRewriteIncludesEnabled();
        if ( clangRewriteIncludes )
        {
            outFullArgs += " -frewrite-includes";
        }
    }
}

// AddAdditionalArgs_Common
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_GCCClang::AddAdditionalArgs_Common( bool isLocal,
                                                                    Args & outFullArgs ) const
{
    if ( m_ForceColoredDiagnostics )
    {
        outFullArgs += " -fdiagnostics-color=always";
    }

    // Add args for source mapping
    if ( ( m_SourceMapping.IsEmpty() == false ) && isLocal )
    {
        const AString& workingDir = FBuild::Get().GetOptions().GetWorkingDir();
        AStackString<> tmp;
        // Using -ffile-prefix-map would be better since that would change not only the file paths in
        // the DWARF debugging information but also in the __FILE__ and related predefined macros, but
        // -ffile-prefix-map is only supported starting with GCC 8 and Clang 10. The -fdebug-prefix-map
        // option is available starting with Clang 3.8 and all modern GCC versions.
        tmp.Format(" \"-fdebug-prefix-map=%s=%s\"", workingDir.Get(), m_SourceMapping.Get());
        outFullArgs += tmp;
    }
}

// ProcessArg_PreparePreprocessedForRemote
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_GCCClang::ProcessArg_PreparePreprocessedForRemote( const AString & token,
                                                                                   size_t & index,
                                                                                   const AString & nextToken,
                                                                                   Args & outFullArgs ) const
{
    // Handle -x language option update
    if ( ProcessArg_XLanguageOption( token, index, nextToken, outFullArgs ) )
    {
        return true;
    }

    // Handle dependency output args (-MD etc)
    if ( ProcessArg_DependencyOption( token, index ) )
    {
        return true;
    }

    return false;
}

// ProcessArg_XLanguageOption
//------------------------------------------------------------------------------
bool CompilerDriver_GCCClang::ProcessArg_XLanguageOption( const AString & token,
                                                          size_t & index,
                                                          const AString & nextToken,
                                                          Args & outFullArgs ) const
{
    // Skip fixup if not enabled.
    //
    // Older versions of Clang (prior to v10) ignore -D directives on the command
    // line if "*-cpp-output" is set, causing compilation to fail when -frewrite-includes
    // is used (the default). We don't know what version of Clang we are using, so the user
    // has to opt-in to this language arg update behavior change to ensure backwards compatibility.
    //
    if ( m_ObjectNode->GetCompiler()->IsClangGCCUpdateXLanguageArgEnabled() == false )
    {
        return false;
    }

    // To avoid preprocesing code a second time we need to update
    // arguments of -x option to use the "cpp-output" variant.
    // We must do this inplace because the argument order matters in this case.
    if ( ( token == "-x" ) && ( nextToken.IsEmpty() == false ) )
    {
        // Save the "-x" token
        outFullArgs += token;
        outFullArgs.AddDelimiter();

        // Change the argument to its "cpp-output" variant.
        const AString & language = nextToken;
        ++index; // consume extra arg
        if ( language == "c" )
        {
            outFullArgs += "cpp-output";
        }
        else if ( ( language == "c++" ) ||
                  ( language == "objective-c" ) ||
                  ( language == "objective-c++" ) )
        {
            outFullArgs += language;
            outFullArgs += "-cpp-output";
        }
        else
        {
            outFullArgs += language;
        }
        outFullArgs.AddDelimiter();
        return true;
    }

    return false;
}

// ProcessArg_DependencyOption
//------------------------------------------------------------------------------
bool CompilerDriver_GCCClang::ProcessArg_DependencyOption( const AString & token,
                                                           size_t & index ) const
{
    // Some integrations (like Unreal) rely on their own parsing of "makefile"
    // style dependency output. This is fine to generate locally as part of
    // preprocessing, but must be removed when compiling preprocessed output.
    // This prevents two type of failures:
    // a) If the -x langauge arg is used, and we update it to reflect that we're
    //    compiling already pre-processed outptu, options will be reported as unused
    //    [-Wunused-command-line-argument]
    // b) When compiling remotely, the directories specified may not exist (and we
    //    wouldn't return the output anyway)
    if ( StripToken( "-MD", token ) )
    {
        return true;
    }
    if ( StripTokenWithArg( "-MF", token, index ) )
    {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
