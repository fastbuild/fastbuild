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
    m_LastBuildTimeMs = 10; // higher default than a file node
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool TextFileNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
TextFileNode::~TextFileNode() = default;

// DetermineNeedToBuild
//------------------------------------------------------------------------------
/*virtual*/ bool TextFileNode::DetermineNeedToBuild( const Dependencies & deps ) const
{  
    if ( m_TextFileAlways )
    {
        FLOG_VERBOSE( "Need to build '%s' (TextFileAlways = true)", GetName().Get() );
        return true;
    }
    return Node::DetermineNeedToBuild( deps );
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult TextFileNode::DoBuild( Job * /*job*/ )
{
    EmitCompilationMessage();

    // Generate the file contents
    AStackString<4096> textFileContents;
    for ( const AString & string : m_TextFileInputStrings )
    {
        textFileContents += string;

        // It's not always safe to include a \r, such as when generating a shell script
        #if defined( __WINDOWS__ )
            textFileContents += "\r\n";
        #else
            textFileContents += '\n';
        #endif
    }

    FileStream stream;
    if ( !stream.Open( GetName().Get(), FileStream::WRITE_ONLY ) )
    {
        FLOG_ERROR( "Could not open '%s' for writing", GetName().Get() );
        return NODE_RESULT_FAILED;
    }
    uint64_t nWritten = stream.WriteBuffer( textFileContents.Get(), textFileContents.GetLength() );
    stream.Close();

    if ( nWritten != textFileContents.GetLength() )
    {
        FLOG_ERROR( "Failed to write all to '%s'", GetName().Get() );
        return NODE_RESULT_FAILED;
    }

    Node::RecordStampFromBuiltFile();

    return NODE_RESULT_OK;
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void TextFileNode::EmitCompilationMessage() const
{
    AStackString<> output;
    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        output += "Txt: ";
        output += GetName();
        output += '\n';
    }

    // output all at once for contiguousness
    FLOG_OUTPUT( output );
}

//------------------------------------------------------------------------------
