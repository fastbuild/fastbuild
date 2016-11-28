// FileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FileNode.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FileNode::FileNode( const AString & fileName, uint32_t controlFlags )
: Node( fileName, Node::FILE_NODE, controlFlags )
{
    ASSERT( fileName.EndsWith( "\\" ) == false );
    ASSERT( ( fileName.FindLast( ':' ) == nullptr ) ||
            ( fileName.FindLast( ':' ) == ( fileName.Get() + 1 ) ) );

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
}

//------------------------------------------------------------------------------
