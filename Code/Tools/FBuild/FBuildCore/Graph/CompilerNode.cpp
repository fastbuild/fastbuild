// CompilerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"


// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( CompilerNode, Node, MetaNone() )
    REFLECT( m_Executable,          "Executable",           MetaFile() )
    REFLECT_ARRAY( m_ExtraFiles,    "ExtraFiles",           MetaOptional() + MetaFile() )
    REFLECT_ARRAY( m_CustomEnvironmentVariables, "CustomEnvironmentVariables",  MetaOptional() )
    REFLECT( m_AllowDistribution,   "AllowDistribution",    MetaOptional() )
    REFLECT( m_AllowResponseFile,   "AllowResponseFile",    MetaOptional() )
    REFLECT( m_ForceResponseFile,   "ForceResponseFile",    MetaOptional() )
    REFLECT( m_VS2012EnumBugFix,    "VS2012EnumBugFix",     MetaOptional() )
    REFLECT( m_ClangRewriteIncludes, "ClangRewriteIncludes", MetaOptional() )
    REFLECT( m_ClangGCCUpdateXLanguageArg, "ClangGCCUpdateXLanguageArg",  MetaOptional() )
    REFLECT( m_ClangFixupUnity_Disable, "ClangFixupUnity_Disable", MetaOptional() )
    REFLECT( m_ExecutableRootPath,  "ExecutableRootPath",   MetaOptional() + MetaPath() )
    REFLECT( m_SimpleDistributionMode,  "SimpleDistributionMode",   MetaOptional() )
    REFLECT( m_CompilerFamilyString,"CompilerFamily",       MetaOptional() )
    REFLECT_ARRAY( m_Environment,   "Environment",          MetaOptional() )
    REFLECT( m_UseLightCache,       "UseLightCache_Experimental", MetaOptional() )
    REFLECT( m_UseRelativePaths,    "UseRelativePaths_Experimental", MetaOptional() )
    REFLECT( m_SourceMapping,       "SourceMapping_Experimental", MetaOptional() )

    // Internal
    REFLECT( m_CompilerFamilyEnum,  "CompilerFamilyEnum",   MetaHidden() )
    REFLECT_STRUCT( m_Manifest,     "Manifest", ToolManifest, MetaHidden() + MetaIgnoreForComparison() )
REFLECT_END( CompilerNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::CompilerNode()
    : Node( AString::GetEmpty(), Node::COMPILER_NODE, Node::FLAG_NONE )
    , m_AllowDistribution( true )
    , m_AllowResponseFile( false )
    , m_ForceResponseFile( false )
    , m_VS2012EnumBugFix( false )
    , m_ClangRewriteIncludes( true )
    , m_ClangGCCUpdateXLanguageArg( false )
    , m_ClangFixupUnity_Disable( false )
    , m_CompilerFamilyString( "auto" )
    , m_CompilerFamilyEnum( static_cast< uint8_t >( CUSTOM ) )
    , m_SimpleDistributionMode( false )
    , m_UseLightCache( false )
    , m_UseRelativePaths( false )
    , m_EnvironmentString( nullptr )
{
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // .Executable
    Dependencies compilerExeFile( 1, false );
    if ( !Function::GetFileNode( nodeGraph, iter, function, m_Executable, ".Executable", compilerExeFile ) )
    {
        return false; // GetFileNode will have emitted an error
    }
    ASSERT( compilerExeFile.GetSize() == 1 ); // Should not be possible to expand to > 1 thing

    // .ExtraFiles
    Dependencies extraFiles( 32, true );
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".ExtraFiles", m_ExtraFiles, extraFiles ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    // If .ExecutableRootPath is not specified, generate it from the .Executable's path
    if ( m_ExecutableRootPath.IsEmpty() )
    {
        const char * lastSlash = m_Executable.FindLast( NATIVE_SLASH );
        if ( lastSlash )
        {
            m_ExecutableRootPath.Assign( m_Executable.Get(), lastSlash + 1 );
        }
    }

    // Check for conflicting files
    AStackString<> relPathExe;
    ToolManifest::GetRelativePath( m_ExecutableRootPath, m_Executable, relPathExe );


    const size_t numExtraFiles = extraFiles.GetSize();
    for ( size_t i=0; i<numExtraFiles; ++i )
    {
        AStackString<> relPathA;
        ToolManifest::GetRelativePath( m_ExecutableRootPath, extraFiles[ i ].GetNode()->GetName(), relPathA );

        // Conflicts with Exe?
        if ( PathUtils::ArePathsEqual( relPathA, relPathExe ) )
        {
            Error::Error_1100_AlreadyDefined( iter, function, relPathA );
            return false;
        }

        // Conflicts with another file?
        for ( size_t j=(i+1); j<numExtraFiles; ++j )
        {
            AStackString<> relPathB;
            ToolManifest::GetRelativePath( m_ExecutableRootPath, extraFiles[ j ].GetNode()->GetName(), relPathB );

            if ( PathUtils::ArePathsEqual( relPathA, relPathB ) )
            {
                Error::Error_1100_AlreadyDefined( iter, function, relPathA );
                return false;
            }
        }
    }

    // Store Static Dependencies
    m_StaticDependencies.SetCapacity( 1 + extraFiles.GetSize() );
    m_StaticDependencies.Append( compilerExeFile );
    m_StaticDependencies.Append( extraFiles );

    if (InitializeCompilerFamily( iter, function ) == false)
    {
        return false;
    }

    // The LightCache is only compatible with MSVC for now
    // - GCC/Clang can be supported when built in include paths can be extracted
    //   and -nostdinc/-nostdinc++ is handled
    if ( m_UseLightCache && ( m_CompilerFamilyEnum != MSVC ) )
    {
        Error::Error_1502_LightCacheIncompatibleWithCompiler( iter, function );
        return false;
    }

    m_Manifest.Initialize( m_ExecutableRootPath, m_StaticDependencies, m_CustomEnvironmentVariables );

    return true;
}

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerNode::IsAFile() const
{
    return false;
}

// InitializeCompilerFamily
//------------------------------------------------------------------------------
bool CompilerNode::InitializeCompilerFamily( const BFFToken * iter, const Function * function )
{
    // Handle auto-detect
    if ( m_CompilerFamilyString.EqualsI( "auto" ) )
    {
        // Normalize slashes to make logic consistent on all platforms
        AStackString<> compiler( GetExecutable() );
        compiler.Replace( '/', '\\' );
        AStackString<> compilerWithoutVersion( compiler.Get() );
        if ( const char * last = compiler.FindLast( '-' ) )
        {
            compilerWithoutVersion.Assign( compiler.Get(), last );
        }

        // MSVC
        if ( compiler.EndsWithI( "\\cl.exe" ) ||
             compiler.EndsWithI( "\\cl" ) ||
             compiler.EndsWithI( "\\icl.exe" ) ||
             compiler.EndsWithI( "\\icl" ) )
        {
            m_CompilerFamilyEnum = MSVC;
            return true;
        }

        // Clang
        if ( compiler.EndsWithI( "clang++.exe" ) ||
             compiler.EndsWithI( "clang++.cmd" ) ||
             compiler.EndsWithI( "clang++" ) ||
             compiler.EndsWithI( "clang.exe" ) ||
             compiler.EndsWithI( "clang.cmd" ) ||
             compiler.EndsWithI( "clang" ) )
        {
            m_CompilerFamilyEnum = CLANG;
            return true;
        }

        // Clang in "cl mode" (MSVC compatibility)
        if ( compiler.EndsWithI( "clang-cl.exe" ) ||
             compiler.EndsWithI( "clang-cl" ) )
        {
            m_CompilerFamilyEnum = CLANG_CL;
            return true;
        }

        // GCC
        if ( compiler.EndsWithI( "gcc.exe" ) ||
             compiler.EndsWithI( "gcc" ) ||
             compilerWithoutVersion.EndsWithI( "gcc" ) ||
             compiler.EndsWithI( "g++.exe" ) ||
             compiler.EndsWithI( "g++" ) ||
             compilerWithoutVersion.EndsWithI( "g++" ) ||
             compiler.EndsWithI( "dcc.exe" ) || // WindRiver
             compiler.EndsWithI( "dcc" ) )      // WindRiver
        {
            m_CompilerFamilyEnum = GCC;
            return true;
        }

        // SNC
        if ( compiler.EndsWithI( "\\ps3ppusnc.exe" ) ||
             compiler.EndsWithI( "\\ps3ppusnc" ) )
        {
            m_CompilerFamilyEnum = SNC;
            return true;
        }

        // CodeWarrior Wii
        if ( compiler.EndsWithI( "\\mwcceppc.exe" ) ||
             compiler.EndsWithI( "\\mwcceppc" ) )
        {
            m_CompilerFamilyEnum = CODEWARRIOR_WII;
            return true;
        }

        // Greenhills WiiU
        if ( compiler.EndsWithI( "\\cxppc.exe" ) ||
             compiler.EndsWithI( "\\cxppc" ) ||
             compiler.EndsWithI( "\\ccppc.exe" ) ||
             compiler.EndsWithI( "\\ccppc" ) )
        {
            m_CompilerFamilyEnum = GREENHILLS_WIIU;
            return true;
        }

        // CUDA
        if ( compiler.EndsWithI( "\\nvcc.exe" ) ||
             compiler.EndsWithI( "\\nvcc" ) )
        {
            m_CompilerFamilyEnum = CUDA_NVCC;
            return true;
        }

        // Qt rcc
        if ( compiler.EndsWith( "rcc.exe" ) ||
             compiler.EndsWith( "rcc" ) )
        {
            m_CompilerFamilyEnum = QT_RCC;
            return true;
        }

        // VBCC
        if ( compiler.EndsWith( "vc.exe" ) ||
             compiler.EndsWith( "vc" ) )
        {
            m_CompilerFamilyEnum = VBCC;
            return true;
        }

        // Orbis wave shader compiler
        if ( compiler.EndsWithI( "orbis-wave-psslc.exe" ) ||
             compiler.EndsWithI( "orbis-wave-psslc" ) )
        {
            m_CompilerFamilyEnum = ORBIS_WAVE_PSSLC;
            return true;
        }

        // C# compiler
        if ( compiler.EndsWithI( "csc.exe" ) ||
             compiler.EndsWithI( "csc" ) )
        {
            m_CompilerFamilyEnum = CSHARP;
            return true;
        }

        // Auto-detect failed
        Error::Error_1500_CompilerDetectionFailed( iter, function, compiler );
        return false;
    }

    // Handle explicitly set compiler types
    if ( m_CompilerFamilyString.EqualsI( "custom" ) )
    {
        m_CompilerFamilyEnum = CUSTOM;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "msvc" ) )
    {
        m_CompilerFamilyEnum = MSVC;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "clang" ) )
    {
        m_CompilerFamilyEnum = CLANG;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "gcc" ) )
    {
        m_CompilerFamilyEnum = GCC;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "clang-cl" ) )
    {
        m_CompilerFamilyEnum = CLANG_CL;
        return true;
    }    
    if ( m_CompilerFamilyString.EqualsI( "snc" ) )
    {
        m_CompilerFamilyEnum = SNC;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "codewarrior-wii" ) )
    {
        m_CompilerFamilyEnum = CODEWARRIOR_WII;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "greenhills-wiiu" ) )
    {
        m_CompilerFamilyEnum = GREENHILLS_WIIU;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "cuda-nvcc" ) )
    {
        m_CompilerFamilyEnum = CUDA_NVCC;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "qt-rcc" ) )
    {
        m_CompilerFamilyEnum = QT_RCC;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "vbcc" ) )
    {
        m_CompilerFamilyEnum = VBCC;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "orbis-wave-psslc" ) )
    {
        m_CompilerFamilyEnum = ORBIS_WAVE_PSSLC;
        return true;
    }
    if ( m_CompilerFamilyString.EqualsI( "csharp" ) )
    {
        m_CompilerFamilyEnum = CSHARP;
        return true;
    }

    // Invalid option
    Error::Error_1501_CompilerFamilyUnrecognized( iter, function, m_CompilerFamilyString );
    return false;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::~CompilerNode()
{
    FREE( (void *)m_EnvironmentString );
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CompilerNode::DoBuild( Job * /*job*/ )
{
    if ( !m_Manifest.DoBuild( m_StaticDependencies ) )
    {
        return Node::NODE_RESULT_FAILED; // Generate will have emitted error
    }

    m_Stamp = m_Manifest.GetTimeStamp();
    return Node::NODE_RESULT_OK;
}

// GetEnvironmentString
//------------------------------------------------------------------------------
const char * CompilerNode::GetEnvironmentString() const
{
    return Node::GetEnvironmentString( m_Environment, m_EnvironmentString );
}

// Migrate
//------------------------------------------------------------------------------
/*virtual*/ void CompilerNode::Migrate( const Node & oldNode )
{
    // Migrate Node level properties
    Node::Migrate( oldNode );

    // Migrate the timestamp/hash info stored for the files in the ToolManifest
    m_Manifest.Migrate( oldNode.CastTo<CompilerNode>()->GetManifest() );
}

//------------------------------------------------------------------------------
