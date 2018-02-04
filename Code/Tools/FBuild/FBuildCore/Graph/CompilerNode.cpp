// CompilerNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CompilerNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/IOStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"


// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( CompilerNode, Node, MetaName( "Executable" ) + MetaFile() )
    REFLECT_ARRAY( m_ExtraFiles,    "ExtraFiles",           MetaOptional() + MetaFile() )
    REFLECT_ARRAY( m_CustomEnvironmentVariables, "CustomEnvironmentVariables",  MetaOptional() )
    REFLECT( m_AllowDistribution,   "AllowDistribution",    MetaOptional() )
    REFLECT( m_VS2012EnumBugFix,    "VS2012EnumBugFix",     MetaOptional() )
    REFLECT( m_ClangRewriteIncludes, "ClangRewriteIncludes", MetaOptional() )
    REFLECT( m_ExecutableRootPath,  "ExecutableRootPath",   MetaOptional() + MetaPath() )
    REFLECT( m_SimpleDistributionMode,  "SimpleDistributionMode",   MetaOptional() )
    REFLECT( m_CompilerFamilyString,"CompilerFamily",       MetaOptional() )

    // Internal
    REFLECT( m_CompilerFamilyEnum,  "CompilerFamilyEnum",   MetaHidden() )
REFLECT_END( CompilerNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::CompilerNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NO_DELETE_ON_FAIL )
    , m_AllowDistribution( true )
    , m_VS2012EnumBugFix( false )
    , m_ClangRewriteIncludes( true )
    , m_CompilerFamilyString( "auto" )
    , m_CompilerFamilyEnum( static_cast< uint8_t >( CUSTOM ) )
    , m_SimpleDistributionMode( false )
{
    m_Type = Node::COMPILER_NODE;
}

// Initialize
//------------------------------------------------------------------------------
bool CompilerNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // TODO:B make this use m_ExtraFiles
    Dependencies extraFiles( 32, true );
    if ( !function->GetNodeList( nodeGraph, iter, ".ExtraFiles", extraFiles, false ) ) // optional
    {
        return false; // GetNodeList will have emitted an error
    }

    if( m_ExecutableRootPath.IsEmpty() )
    {
        const char * lastSlash = m_Name.FindLast( NATIVE_SLASH );
        if ( lastSlash )
        {
            m_ExecutableRootPath.Assign( m_Name.Get(), lastSlash + 1 );
        }
    }

    // Check for conflicting files
    AStackString<> relPathExe;
    ToolManifest::GetRelativePath( m_ExecutableRootPath, m_Name, relPathExe );

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

    m_StaticDependencies = extraFiles;

    return InitializeCompilerFamily( iter, function );
}

// InitializeCompilerFamily
//------------------------------------------------------------------------------
bool CompilerNode::InitializeCompilerFamily( const BFFIterator & iter, const Function * function )
{
    // Handle auto-detect
    if ( m_CompilerFamilyString.EqualsI( "auto" ) )
    {
        // Normalize slashes to make logic consistent on all platforms
        AStackString<> compiler( m_Name );
        compiler.Replace( '/', '\\' );

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
             compiler.EndsWithI( "clang++" ) ||
             compiler.EndsWithI( "clang.exe" ) ||
             compiler.EndsWithI( "clang" ) ||
             compiler.EndsWithI( "clang-cl.exe" ) ||
             compiler.EndsWithI( "clang-cl" ) )
        {
            m_CompilerFamilyEnum = CLANG;
            return true;
        }

        // GCC
        if ( compiler.EndsWithI( "gcc.exe" ) ||
             compiler.EndsWithI( "gcc" ) ||
             compiler.EndsWithI( "g++.exe" ) ||
             compiler.EndsWithI( "g++" ) ||
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

    // Invalid option
    Error::Error_1501_CompilerFamilyUnrecognized( iter, function, m_CompilerFamilyString );
    return false;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerNode::~CompilerNode() = default;

// DetermineNeedToBuild
//------------------------------------------------------------------------------
bool CompilerNode::DetermineNeedToBuild( bool forceClean ) const
{
    if ( forceClean )
    {
        return true;
    }

    // Building for the first time?
    if ( m_Stamp == 0 )
    {
        return true;
    }

    // check primary file
    const uint64_t fileTime = FileIO::GetFileLastWriteTime( m_Name );
    if ( fileTime > m_Stamp )
    {
        return true;
    }

    // check additional files
    for ( const auto & dep : m_StaticDependencies )
    {
        if ( dep.GetNode()->GetStamp() > m_Stamp )
        {
            return true;
        }
    }

    return false;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CompilerNode::DoBuild( Job * job )
{
    // ensure our timestamp is updated (Generate requires this)
    FileNode::DoBuild( job );

    if ( !m_Manifest.Generate( this, m_ExecutableRootPath, m_StaticDependencies, m_CustomEnvironmentVariables ) )
    {
        return Node::NODE_RESULT_FAILED; // Generate will have emitted error
    }

    m_Stamp = Math::Max( m_Stamp, m_Manifest.GetTimeStamp() );
    return Node::NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CompilerNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    CompilerNode * cn = nodeGraph.CreateCompilerNode( name );

    if ( cn->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }
    cn->m_Manifest.Deserialize( stream, false ); // false == not remote
    return cn;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CompilerNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
    m_Manifest.Serialize( stream );
}

//------------------------------------------------------------------------------
