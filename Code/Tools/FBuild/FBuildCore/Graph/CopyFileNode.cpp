// CopyFileNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CopyFileNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( CopyFileNode, Node, MetaNone() )
    REFLECT(        m_Source,                   "Source",                   MetaFile() )
    REFLECT(        m_Dest,                     "Dest",                     MetaPath() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,  "PreBuildDependencies",     MetaOptional() + MetaFile() + MetaAllowNonFile() )
    REFLECT( m_SymlinkBehavior, "SymlinkBehavior", MetaOptional() )
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
/*virtual*/ bool CopyFileNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
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

    if ( FileIO::ParseSymlinkBehavior( m_SymlinkBehavior ) == FileIO::INVALID_SYM_LINK_BEHAVIOR )
    {
        Error::Error_1104_TargetNotDefined( iter, function, "SymlinkBehavior", m_SymlinkBehavior );
        return false;
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
    if ( FileIO::FileCopy( GetSourceNode()->GetName().Get(), m_Name.Get(), FileIO::ParseSymlinkBehavior( m_SymlinkBehavior ) ) == false )
    {
        FLOG_ERROR( "Copy failed. Error: %s Target: '%s'", LAST_ERROR_STR, GetName().Get() );
        return NODE_RESULT_FAILED; // copy failed
    }

    if ( FileIO::SetReadOnly( m_Name.Get(), false ) == false )
    {
        FLOG_ERROR( "Copy read-only flag set failed. Error: %s Target: '%s'", LAST_ERROR_STR, GetName().Get() );
        return NODE_RESULT_FAILED; // failed to remove read-only
    }

    // Ensure the dst file's "last modified" time is equal to or newer than the source
    uint64_t srcStamp = FileIO::GetFileLastWriteTime( GetSourceNode()->GetName() );
    uint64_t dstStamp = FileIO::GetFileLastWriteTime( m_Name );
    ASSERT( srcStamp && dstStamp );
    if ( dstStamp < srcStamp )
    {
        // File system copy didn't transfer the "last modified" time, so set it explicitly
        if ( FileIO::SetFileLastWriteTime( m_Name, srcStamp ) == false )
        {
            FLOG_ERROR( "Copy set last write time failed. Error: %s Target: '%s'", LAST_ERROR_STR, GetName().Get() );
            m_Stamp = 0;
            return NODE_RESULT_FAILED; // failed to set the time
        }
        dstStamp = srcStamp;
    }
    m_Stamp = dstStamp;
    return NODE_RESULT_OK;
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
    if ( !m_SymlinkBehavior.IsEmpty() )
    {
        output += " (";
        output += m_SymlinkBehavior;
        output += ")";
    }
    output += '\n';
    FLOG_BUILD_DIRECT( output.Get() );
}

//------------------------------------------------------------------------------
