// CompilerInfoNode.cpp
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "CompilerInfoNode.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"

// Core
#include "Core/Env/ErrorFormat.h"
#include "Core/Process/Process.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( CompilerInfoNode, Node, MetaNone() )
    // Internal
    REFLECT( m_Compiler, MetaHidden() )
    REFLECT( m_NoStdInc, MetaHidden() )
    REFLECT( m_NoStdIncPP, MetaHidden() )
    REFLECT( m_BuiltinIncludePaths, MetaHidden() )
REFLECT_END( CompilerInfoNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerInfoNode::CompilerInfoNode()
    : Node( Node::COMPILER_INFO_NODE )
{
}

//------------------------------------------------------------------------------
/*virtual*/ bool CompilerInfoNode::Initialize( NodeGraph & /*nodeGraph*/,
                                               const BFFToken * /*iter*/,
                                               const Function * /*function*/ )
{
    ASSERT( m_Compiler ); // Should be set by creator before Initialize

    // If the underlying compiler changes, we'll need to re-evaluate
    m_StaticDependencies.Add( m_Compiler );

    return true;
}

//------------------------------------------------------------------------------
/*virtual*/ bool CompilerInfoNode::IsAFile() const
{
    return false;
}

//------------------------------------------------------------------------------
CompilerInfoNode::~CompilerInfoNode() = default;

//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CompilerInfoNode::DoBuild( Job * /*job*/ )
{
    // If all includes are suppressed, we can skip compiler invocation
    if ( m_NoStdInc && m_NoStdIncPP )
    {
        m_BuiltinIncludePaths.Clear();
        m_Stamp = 0x1; // Non-zero to avoid rebuilds
        return BuildResult::eOk;
    }

    // We need to invoke the compiler to get a list of include paths
    const AString & compiler = m_Compiler->GetExecutable();

    // The args we need to pass depened on the setings used
    AStackString args;
    args += " -v"; // 'verbose' - used to see include paths
    args += " -fsyntax-only"; // Minimize work done processing empty input
    args += " -Werror -Wfatal-errors"; // Make warnings fatal errors
    if ( m_NoStdInc )
    {
        args += " -nostdinc";
    }
    if ( m_NoStdIncPP )
    {
        args += " -nostdinc++";
    }

    // Specify the language
    const bool isCPP = true; // TODO:A Determine this properly
    if ( m_Compiler->GetCompilerFamily() == CompilerNode::CompilerFamily::CLANG_CL )
    {
        // Clang CL uses MSVC style args to specify language
        args += isCPP ? " /Tp" : " /Tc";
    }
    else
    {
        // Clang and GCC use the same style args to specify language
        args += isCPP ? " -x c++" : " -x c";
    }

    // Input empty data (we don't want to actually compile anything)
#if defined( __WINDOWS__ )
    args += " NUL";
#else
    args += " /dev/null";
#endif

    EmitCompilationMessage( args );

    // Spawn process and capture all output
    Process p( FBuild::Get().GetAbortBuildPointer() );
    AStackString stdOut;
    AStackString stdErr;
    if ( !p.Spawn( compiler.Get(),
                   args.Get(),
                   nullptr, // Current working dir
                   m_Compiler->GetEnvironmentString() ) ||
         !p.ReadAllData( stdOut, stdErr ) )
    {
        if ( p.HasAborted() )
        {
            return BuildResult::eAborted;
        }

        FLOG_ERROR( "Failed to start compiler to extract CompilerInfo\n"
                    " - Error : %s\n"
                    " - Target: '%s'\n",
                    LAST_ERROR_STR,
                    GetName().Get() );
        return BuildResult::eFailed;
    }

    // Ensure it completed without error
    const int32_t result = p.WaitForExit();
    if ( p.HasAborted() )
    {
        return BuildResult::eAborted;
    }
    if ( result != 0 )
    {
        FLOG_ERROR( "Compiler invocation for CompilerInfo failed.\n"
                    " - Result: %i\n"
                    " - Target: '%s'\n",
                    result,
                    GetName().Get() );
        return BuildResult::eFailed;
    }

    // The output we want is in stdErr
    ASSERT( stdErr.IsEmpty() == false );

    // The includes are between these lines
    const char * const includesStartStringA = "#include \"...\" search starts here:";
    const char * const includesStartStringB = "#include <...> search starts here:";
    const char * const includesEndString = "End of search list.";

    // Search for the first include output start section
    const char * const posA = stdErr.Find( includesStartStringA );
    const char * const posB = stdErr.Find( includesStartStringB );
    const char * pos = posA;
    if ( posA )
    {
        // Take lower of non-null pointers
        if ( posB && ( posB < posA ) )
        {
            pos = posB;
        }
    }
    else
    {
        pos = posB;
    }
    if ( pos == nullptr )
    {
        FLOG_ERROR( "Compiler output unexpected extracting CompilerInfo.\n"
                    " - Problem: Failed to find \"%s\" or \"%s\"\n"
                    " - Target: '%s'\n",
                    includesStartStringA,
                    includesStartStringB,
                    GetName().Get() );
        return BuildResult::eFailed;
    }
    // Remove stuff before that
    stdErr.Trim( static_cast<uint32_t>( pos - stdErr.Get() ), 0 );

    // Split the includes section into lines
    StackArray<AString> lines;
    stdErr.Tokenize( lines, '\n' );
    for ( AString & line : lines )
    {
        if ( line.BeginsWith( ' ' ) )
        {
            line.TrimStart( ' ' );
            line.TrimEnd( ' ' );
            m_BuiltinIncludePaths.EmplaceBack( Move( line ) );
        }
        else if ( line.BeginsWith( includesEndString ) )
        {
            break; // All done. Ignore lines that may be after
        }
        else if ( line.BeginsWith( includesStartStringA ) ||
                  line.BeginsWith( includesStartStringB ) )
        {
            continue; // Skip first line
        }
        else
        {
            FLOG_ERROR( "Compiler output unexpected extracting CompilerInfo.\n"
                        " - Problem: Encountered unexepected line in include section\n"
                        " - Line   : '%s'\n"
                        " - Target : '%s'\n",
                        line.Get(),
                        GetName().Get() );
            return BuildResult::eFailed;
        }
    }

    return BuildResult::eOk;
}

//------------------------------------------------------------------------------
void CompilerInfoNode::EmitCompilationMessage( const AString & args ) const
{
    // print basic or detailed output, depending on options
    // we combine everything into one string to ensure it is contiguous in
    // the output
    AStackString output;
    if ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        output += "CompilerInfo: ";
        output += GetName();
        output += '\n';
    }
    if ( FBuild::IsValid() && FBuild::Get().GetOptions().m_ShowCommandLines )
    {
        output += m_Compiler->GetExecutable();
        output += ' ';
        output += args;
        output += '\n';
    }
    if ( output.IsEmpty() == false )
    {
        FLOG_OUTPUT( output );
    }
}

//------------------------------------------------------------------------------
