// FileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FileNode.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Containers/AutoPtr.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FileNode::FileNode( const AString & fileName, uint32_t controlFlags )
: Node( fileName, Node::FILE_NODE, controlFlags )
{
    ASSERT( fileName.EndsWith( "\\" ) == false );
    #if defined( __WINDOWS__ )
        ASSERT( ( fileName.FindLast( ':' ) == nullptr ) ||
                ( fileName.FindLast( ':' ) == ( fileName.Get() + 1 ) ) );
    #endif

    m_LastBuildTimeMs = 1; // very little work required
}

// DESTRUCTOR
//------------------------------------------------------------------------------
FileNode::~FileNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult FileNode::DoBuild( Job * UNUSED( job ) )
{
    m_Stamp = FileIO::GetFileLastWriteTime( m_Name );
    return NODE_RESULT_OK;
}

// HandleWarningsMSVC
//------------------------------------------------------------------------------
void FileNode::HandleWarningsMSVC( Job * job, const AString & name, const char * data, uint32_t dataSize )
{
    if ( ( data == nullptr ) || ( dataSize == 0 ) )
    {
        return;
    }

    // Are there any warnings? (string is ok even in non-English)
    if ( strstr( data, ": warning " ) )
    {
        const bool treatAsWarnings = true;
        DumpOutput( job, data, dataSize, name, treatAsWarnings );
    }
}

// DumpOutput
//------------------------------------------------------------------------------
void FileNode::DumpOutput( Job * job, const char * data, uint32_t dataSize, const AString & name, bool treatAsWarnings )
{
    if ( ( data != nullptr ) && ( dataSize > 0 ) )
    {
        Array< AString > exclusions( 2, false );
        exclusions.Append( AString( "Note: including file:" ) );
        exclusions.Append( AString( "#line" ) );

        AStackString<> msg;
        msg.Format( "%s: %s\n", treatAsWarnings ? "WARNING" : "PROBLEM", name.Get() );

        AutoPtr< char > mem( (char *)Alloc( dataSize + msg.GetLength() ) );
        memcpy( mem.Get(), msg.Get(), msg.GetLength() );
        memcpy( mem.Get() + msg.GetLength(), data, dataSize );

        Node::DumpOutput( job, mem.Get(), dataSize + msg.GetLength(), &exclusions );
    }
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * FileNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>,  fileName );

    Node * n = nodeGraph.CreateFileNode( fileName );
    ASSERT( n );
    return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void FileNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    #if defined( DEBUG )
        MarkAsSaved();
    #endif
}

//------------------------------------------------------------------------------
