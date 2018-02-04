// RemoveDirNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "RemoveDirNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Env/Env.h"
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
    : Node( AString::GetEmpty(), Node::REMOVE_DIR_NODE, Node::FLAG_NONE )
    , m_RemovePathsRecurse( true )
{
    m_RemovePatterns.Append( AStackString<>( "*" ) );
}

// Initialize
//------------------------------------------------------------------------------
bool RemoveDirNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // Convert RemovePaths paths to DirectoryListNodes
    Dependencies fileListDeps( m_RemovePaths.GetSize() );
    if ( !function->GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              m_RemovePaths,
                                              m_RemoveExcludePaths,
                                              Array< AString >(), // unused FilesToExclude
                                              Array< AString >(), // unused ExcludePatterns
                                              m_RemovePathsRecurse,
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
/*virtual*/ Node::BuildResult RemoveDirNode::DoBuild( Job * UNUSED( job ) )
{
    ASSERT( !m_StaticDependencies.IsEmpty() );

    m_Stamp = 0; // Trigger DoBuild() every time

    // Iterate all the DirectoryListNodes
    const Dependency * const depEnd = m_StaticDependencies.End();
    for ( const Dependency * dep = m_StaticDependencies.Begin();
          dep != depEnd;
          ++dep )
    {
        // Grab the files
        DirectoryListNode * dln = dep->GetNode()->CastTo< DirectoryListNode >();
        const Array< FileIO::FileInfo > & files = dln->GetFiles();
        const FileIO::FileInfo * const fEnd = files.End();
        for ( const FileIO::FileInfo * fIt = files.Begin();
              fIt != fEnd;
              ++fIt )
        {
            // source file (full path)
            const AString & srcFile = fIt->m_Name;

            // remove the file
            if ( FileIO::FileDelete( srcFile.Get() ) == false )
            {
                FLOG_ERROR( "Remove failed (error %i) '%s'", Env::GetLastErr(), srcFile.Get() );
                return NODE_RESULT_FAILED; // remove failed
            }

            // we combine everything into one string to ensure it is contiguous in
            // the output
            AStackString<> output;
            output += "Remove: ";
            output += srcFile;
            output += '\n';
            FLOG_BUILD_DIRECT( output.Get() );
        }
    }

    return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * RemoveDirNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    RemoveDirNode * node = nodeGraph.CreateRemoveDirNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }

    return node;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void RemoveDirNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

//------------------------------------------------------------------------------
