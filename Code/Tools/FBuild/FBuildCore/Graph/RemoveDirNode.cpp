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
    REFLECT_ARRAY( m_RemoveExcludePaths,        "RemoveExcludePaths",   MetaOptional() + MetaPath() )
    REFLECT_ARRAY( m_PreBuildDependencyNames,   "PreBuildDependencies", MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( RemoveDirNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
RemoveDirNode::RemoveDirNode()
    : Node( AString::GetEmpty(), Node::REMOVE_DIR_NODE, Node::FLAG_ALWAYS_BUILD )
    , m_RemovePathsRecurse( true )
{
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
                                              Array< AString >(), // unused FilesToExclude
                                              Array< AString >(), // unused ExcludePatterns
                                              m_RemovePathsRecurse,
                                              false, // Don't include read-only status in hash
                                              &m_RemovePatterns,
                                              "RemovePaths",
                                              fileListDeps ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }

    // Store dependencies
    m_StaticDependencies.Append( fileListDeps );

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
        // Grab the files
        const DirectoryListNode * dln = dep.GetNode()->CastTo< DirectoryListNode >();
        const Array< FileIO::FileInfo > & files = dln->GetFiles();
        for ( const FileIO::FileInfo & fileInfo : files )
        {
            // source file (full path)
            const AString & srcFile = fileInfo.m_Name;

            // remove the file
            if ( FileIO::FileDelete( srcFile.Get() ) == false )
            {
                FLOG_ERROR( "Remove failed. Error: %s Target: '%s'", LAST_ERROR_STR, srcFile.Get() );
                return NODE_RESULT_FAILED; // remove failed
            }

            // we combine everything into one string to ensure it is contiguous in
            // the output
            AStackString<> output;
            if ( FBuild::Get().GetOptions().m_ShowCommandSummary )
            {
                output += "Remove: ";
                output += srcFile;
                output += '\n';
                FLOG_OUTPUT( output );
            }
        }
    }

    return NODE_RESULT_OK;
}

//------------------------------------------------------------------------------
