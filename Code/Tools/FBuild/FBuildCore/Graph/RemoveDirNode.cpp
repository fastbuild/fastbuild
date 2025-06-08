// RemoveDirNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "RemoveDirNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/Strings/AStackString.h"

// Reflection
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( RemoveDirNode, Node, MetaNone() )
    REFLECT_ARRAY( m_RemovePaths,               "RemovePaths",          MetaPath() )
    REFLECT_ARRAY( m_RemovePatterns,            "RemovePatterns",       MetaOptional() )
    REFLECT(       m_RemovePathsRecurse,        "RemovePathsRecurse",   MetaOptional() )
    REFLECT(       m_RemoveDirs,                "RemoveDirs",           MetaOptional() )
    REFLECT(       m_RemoveRootDir,             "RemoveRootDir",        MetaOptional() )
    REFLECT_ARRAY( m_RemoveExcludePaths,        "RemoveExcludePaths",   MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_RemoveExcludeFiles,        "RemoveExcludeFiles",   MetaOptional() + MetaFile() )
    REFLECT_ARRAY( m_PreBuildDependencyNames,   "PreBuildDependencies", MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( RemoveDirNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
RemoveDirNode::RemoveDirNode()
    : Node( Node::REMOVE_DIR_NODE )
{
    m_ControlFlags = Node::FLAG_ALWAYS_BUILD;
    m_RemovePatterns.EmplaceBack( "*" );
}

// Initialize
//------------------------------------------------------------------------------
/*virtual*/ bool RemoveDirNode::Initialize( NodeGraph & nodeGraph, const BFFToken * iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // Convert RemovePaths paths to DirectoryListNodes
    Dependencies fileListDeps( m_RemovePaths.GetSize() );
    if ( !Function::GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              function,
                                              m_RemovePaths,
                                              m_RemoveExcludePaths,
                                              m_RemoveExcludeFiles,
                                              Array<AString>(), // unused ExcludePatterns
                                              m_RemovePathsRecurse,
                                              false, // Don't include read-only status in hash
                                              m_RemoveDirs,
                                              &m_RemovePatterns,
                                              "RemovePaths",
                                              fileListDeps ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    // Store dependencies
    m_StaticDependencies.Add( fileListDeps );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
RemoveDirNode::~RemoveDirNode() = default;

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool RemoveDirNode::IsAFile() const
{
    return false;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult RemoveDirNode::DoBuild( Job * /*job*/ )
{
    ASSERT( !m_StaticDependencies.IsEmpty() );

    // Iterate all the DirectoryListNodes
    for ( const Dependency & dep : m_StaticDependencies )
    {
        const DirectoryListNode * dln = dep.GetNode()->CastTo<DirectoryListNode>();

        // Delete files
        for ( const FileIO::FileInfo & fileInfo : dln->GetFiles() )
        {
            if ( !FileDelete( fileInfo.m_Name ) )
            {
                return BuildResult::eFailed; // remove failed
            }
        }

        // Delete folders
        if ( m_RemoveDirs )
        {
            const Array<AString> & dirs = dln->GetDirectories();
            for ( int32_t i = static_cast<int32_t>( dirs.GetSize() - 1 ); i >= 0; --i )
            {
                if ( !DirectoryDelete( dirs[ static_cast<size_t>( i ) ] ) )
                {
                    return BuildResult::eFailed; // remove failed
                }
            }
        }

        // Delete root folder
        if ( m_RemoveDirs && m_RemoveRootDir )
        {
            // Sub-directories come from a directory lists, so we know they
            // exist, but the root may not exist
            if ( FileIO::DirectoryExists( dln->GetPath() ) )
            {
                if ( !DirectoryDelete( dln->GetPath() ) )
                {
                    return BuildResult::eFailed; // remove failed
                }
            }
        }
    }

    return BuildResult::eOk;
}

//------------------------------------------------------------------------------
bool RemoveDirNode::FileDelete( const AString & file ) const
{
    // remove the file
    if ( FileIO::FileDelete( file.Get() ) == false )
    {
        FLOG_ERROR( "Remove failed. Error: %s Target: '%s'", LAST_ERROR_STR, file.Get() );
        return false;
    }

    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        FLOG_OUTPUT( "Remove: %s\n", file.Get() );
    }
    return true;
}

//------------------------------------------------------------------------------
bool RemoveDirNode::DirectoryDelete( const AString & dir ) const
{
    if ( FileIO::DirectoryDelete( dir ) == false )
    {
        FLOG_ERROR( "Remove failed. Error: %s Target: '%s'", LAST_ERROR_STR, dir.Get() );
        return false;
    }

    if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
    {
        FLOG_OUTPUT( "Remove: %s\n", dir.Get() );
    }
    return true;
}

//------------------------------------------------------------------------------
