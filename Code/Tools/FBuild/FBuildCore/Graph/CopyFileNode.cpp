// CopyFileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CopyFileNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( CopyFileNode, Node, MetaNone() )
    REFLECT(        m_Source,                   "Source",                   MetaFile() )
    REFLECT(        m_Dest,                     "Dest",                     MetaPath() )
    REFLECT(        m_SourceBasePath,           "SourceBasePath",           MetaOptional() + MetaPath() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,  "PreBuildDependencies",     MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( CopyFileNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
CopyFileNode::CopyFileNode()
: FileNode( AString::GetEmpty(), Node::FLAG_NONE )
{
    m_Type = Node::COPY_FILE_NODE;
}

// Initialize
//------------------------------------------------------------------------------
bool CopyFileNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // Get node for Source of copy
    if ( !Function::GetNodeList( nodeGraph, iter, function, ".Source", m_Source, m_StaticDependencies ) )
    {
        return false; // GetNodeList will have emitted an error
    }

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CopyFileNode::~CopyFileNode() = default;

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CopyFileNode::DoBuild( Job * UNUSED( job ) )
{
    EmitCopyMessage();

    // copy the file
    if ( FileIO::FileCopy( GetSourceNode()->GetName().Get(), m_Name.Get() ) == false )
    {
        FLOG_ERROR( "Copy failed (error %i) '%s'", Env::GetLastErr(), GetName().Get() );
        return NODE_RESULT_FAILED; // copy failed
    }

    if ( FileIO::SetReadOnly( m_Name.Get(), false ) == false )
    {
        FLOG_ERROR( "Copy read-only flag set failed (error %i) '%s'", Env::GetLastErr(), GetName().Get() );
        return NODE_RESULT_FAILED; // failed to remove read-only
    }

    // update the file's "last modified" time to be equal to the src file
    // TODO:C Use m_SourceFile->GetTimeStamp() when sibling nodes are supported
    // (because we use PreBuildDependencies as a work around, the source time stamp
    // can be taken before the copy is done, so get the current time again here for now)
    //m_Stamp = m_SourceFile->GetStamp();
    m_Stamp = FileIO::GetFileLastWriteTime( GetSourceNode()->GetName() );
    ASSERT( m_Stamp );
    if ( FileIO::SetFileLastWriteTime( m_Name, m_Stamp ) == false )
    {
        FLOG_ERROR( "Copy set last write time failed (error %i) '%s'", Env::GetLastErr(), GetName().Get() );
        m_Stamp = 0;
        return NODE_RESULT_FAILED; // failed to set the time
    }

    return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CopyFileNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    CopyFileNode * node = nodeGraph.CreateCopyFileNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }
    return node;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CopyFileNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

// EmitCompilationMessage
//------------------------------------------------------------------------------
void CopyFileNode::EmitCopyMessage() const
{
    // we combine everything into one string to ensure it is contiguous in
    // the output
    AStackString<> output;
    output += "Copy: ";
    output += m_StaticDependencies[ 0 ].GetNode()->GetName();
    output += " -> ";
    output += GetName();
    output += '\n';
    FLOG_BUILD_DIRECT( output.Get() );
}

//------------------------------------------------------------------------------
