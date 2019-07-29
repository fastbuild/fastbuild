// TextFileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "TextFileNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Math/Conversions.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Process.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( TextFileNode, Node, MetaName( "TextFileOutput" ) + MetaFile() )
    REFLECT_ARRAY( m_TextFileInputStrings, "TextFileInputStrings", MetaNone() )
    REFLECT( m_TextFileAlways, "TextFileAlways", MetaOptional() )
    REFLECT( m_Hidden, "Hidden", MetaOptional() )
    REFLECT_ARRAY( m_PreBuildDependencyNames, "PreBuildDependencies", MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( TextFileNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
TextFileNode::TextFileNode()
    : FileNode( AString::GetEmpty(), Node::FLAG_NONE )
    , m_TextFileAlways( false )
{
    m_Type = TEXT_FILE_NODE;
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool TextFileNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // Store Static Dependencies
    m_StaticDependencies.Clear();

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
TextFileNode::~TextFileNode() = default;

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool TextFileNode::DoDynamicDependencies( NodeGraph & UNUSED( nodeGraph ), bool UNUSED( forceClean ) )
{
    // clear dynamic deps from previous passes
    m_DynamicDependencies.Clear();

    return true;
}

// DetermineNeedToBuild
//------------------------------------------------------------------------------
/*virtual*/ bool TextFileNode::DetermineNeedToBuild( bool forceClean ) const
{  
    if ( m_TextFileAlways )
    {
        FLOG_INFO( "Need to build '%s' (TextFileAlways = true)", GetName().Get() );
        return true;
    }
    return Node::DetermineNeedToBuild( forceClean );
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TextFileNode::DoBuild( Job * UNUSED( job ) )
{
    EmitCompilationMessage();

    // Generate the file contents for comparing with later
    size_t totalSize = 1;
    for ( size_t i = 0; i < m_TextFileInputStrings.GetSize(); ++i )
    {
      totalSize += m_TextFileInputStrings[ i ].GetLength() + 1;
    }
    m_TextFileContents.Clear();
    m_TextFileContents.SetReserved( totalSize );
    for ( size_t i = 0; i < m_TextFileInputStrings.GetSize(); ++i )
    {
      m_TextFileContents += m_TextFileInputStrings[ i ];
      // It's not always safe to include a \r, such as when generating a shell script
#if defined( __WINDOWS__ )
      m_TextFileContents += "\r\n";
#else
      m_TextFileContents += '\n';
#endif
    }

    FileStream stream;
    if ( !stream.Open( GetName().Get(), FileStream::WRITE_ONLY ) )
    {
      FLOG_ERROR( "Could not open '%s' for writing", GetName().Get() );
      return NODE_RESULT_FAILED;
    }
    uint64_t nWritten = stream.WriteBuffer( m_TextFileContents.Get(), m_TextFileContents.GetLength() );
    stream.Close();

    if ( nWritten != m_TextFileContents.GetLength() )
    {
      FLOG_ERROR( "Failed to write all to '%s'", GetName().Get() );
      return NODE_RESULT_FAILED;
    }

    uint64_t dstStamp = FileIO::GetFileLastWriteTime( m_Name );
    m_Stamp = dstStamp;

    return NODE_RESULT_OK;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void TextFileNode::EmitCompilationMessage() const
{
    // basic info
    AStackString< 2048 > output;
    output += "Txt: ";
    output += GetName();
    output += '\n';

    // Should this node spew file contents in verbose mode?
    // if ( FLog::ShowInfo() || FBuild::Get().GetOptions().m_ShowCommandLines )
    // {
    //     output += m_TextFileContents;
    //     output += '\n';
    // }

    // output all at once for contiguousness
    FLOG_BUILD_DIRECT( output.Get() );
}


//------------------------------------------------------------------------------
