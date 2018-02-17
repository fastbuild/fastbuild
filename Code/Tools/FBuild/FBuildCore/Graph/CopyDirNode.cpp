// CopyDirNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "CopyDirNode.h"

#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/Graph/CopyFileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Strings/AStackString.h"

// REFLECTION
//------------------------------------------------------------------------------
REFLECT_NODE_BEGIN( CopyDirNode, Node, MetaNone() )
    REFLECT_ARRAY(  m_SourcePaths,              "SourcePaths",              MetaPath() )
    REFLECT(        m_Dest,                     "Dest",                     MetaPath() )
    REFLECT_ARRAY(  m_SourcePathsPattern,       "SourcePathsPattern",       MetaOptional() )
    REFLECT_ARRAY(  m_SourceExcludePaths,       "SourceExcludePaths",       MetaOptional() + MetaPath() )
    REFLECT(        m_SourcePathsRecurse,       "SourcePathsRecurse",       MetaOptional() )
    REFLECT_ARRAY(  m_PreBuildDependencyNames,  "PreBuildDependencies",     MetaOptional() + MetaFile() + MetaAllowNonFile() )
REFLECT_END( CopyDirNode )

// CONSTRUCTOR
//------------------------------------------------------------------------------
CopyDirNode::CopyDirNode()
: Node( AString::GetEmpty(), Node::COPY_DIR_NODE, Node::FLAG_NONE )
{
}

// Initialize
//------------------------------------------------------------------------------
bool CopyDirNode::Initialize( NodeGraph & nodeGraph, const BFFIterator & iter, const Function * function )
{
    // .PreBuildDependencies
    if ( !InitializePreBuildDependencies( nodeGraph, iter, function, m_PreBuildDependencyNames ) )
    {
        return false; // InitializePreBuildDependencies will have emitted an error
    }

    // .CompilerInputPath
    Dependencies sourcePaths;
    if ( !function->GetDirectoryListNodeList( nodeGraph,
                                              iter,
                                              m_SourcePaths,
                                              m_SourceExcludePaths,
                                              Array< AString >(),     // Unsupported: Excluded files
                                              Array< AString >(),    // Unsupported: Excluded patterns
                                              m_SourcePathsRecurse,
                                              &m_SourcePathsPattern,
                                              "SourcePaths",
                                              sourcePaths ) )
    {
        return false; // GetDirectoryListNodeList will have emitted an error
    }
    ASSERT( sourcePaths.GetSize() == m_SourcePaths.GetSize() );

    // Store dependencies
    m_StaticDependencies.SetCapacity( sourcePaths.GetSize() );
    m_StaticDependencies.Append( sourcePaths );

    return true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
CopyDirNode::~CopyDirNode() = default;

// IsAFile
//------------------------------------------------------------------------------
/*virtual*/ bool CopyDirNode::IsAFile() const
{
    return false;
}

// DoDynamicDependencies
//------------------------------------------------------------------------------
/*virtual*/ bool CopyDirNode::DoDynamicDependencies( NodeGraph & nodeGraph, bool forceClean )
{
    (void)forceClean; // dynamic deps are always re-added here, so this is meaningless

    m_DynamicDependencies.Clear();

    ASSERT( !m_StaticDependencies.IsEmpty() );

    Array< AString > preBuildDependencyNames( m_PreBuildDependencies.GetSize(), false );
    for ( const auto & dep : m_PreBuildDependencies )
    {
        preBuildDependencyNames.Append( dep.GetNode()->GetName() );
    }

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
            // Create a CopyFileNode for each dynamically discovered file

            // source file (full path)
            const AString & srcFile = fIt->m_Name;

            // source file (relative to base path)
            const AStackString<> srcFileRel( srcFile.Get() + dln->GetPath().GetLength() );

            // source file (as a node)
            Node * srcFileNode = nodeGraph.FindNode( srcFile );
            if ( srcFileNode == nullptr )
            {
                srcFileNode = nodeGraph.CreateFileNode( srcFile );
            }
            else if ( srcFileNode->IsAFile() == false )
            {
                FLOG_ERROR( "CopyDir() Node '%s' is not a FileNode (type: %s)", srcFile.Get(), srcFileNode->GetTypeName() );
                return false;
            }

            // generate dest file name
            AStackString<> dstFile( m_Dest );
            dstFile += srcFileRel;

            // make sure dest doesn't already exist
            Node * n = nodeGraph.FindNode( dstFile );
            if ( n == nullptr )
            {
                CopyFileNode * copyFileNode = nodeGraph.CreateCopyFileNode( dstFile );
                copyFileNode->m_Source = srcFileNode->GetName();
                copyFileNode->m_PreBuildDependencyNames = preBuildDependencyNames; // inherit PreBuildDependencies
                BFFIterator iter;
                if ( !copyFileNode->Initialize( nodeGraph, iter, nullptr ) )
                {
                    return false; // Initialize will have emitted an error
                }
                n = copyFileNode;
            }
            else if ( n->GetType() != Node::COPY_FILE_NODE )
            {
                FLOG_ERROR( "Node '%s' is not a CopyFileNode (type: %s)", n->GetName().Get(), n->GetTypeName() );
                return false;
            }
            else
            {
                CopyFileNode * cn = n->CastTo< CopyFileNode >();
                if ( srcFileNode != cn->GetSourceNode() )
                {
                    FLOG_ERROR( "Conflicting objects found during CopyDir:\n"
                                " File A: %s\n"
                                " File B: %s\n"
                                " Both copy to: %s\n",
                                srcFile.Get(),
                                cn->GetSourceNode()->GetName().Get(),
                                dstFile.Get() );
                    return false;
                }
            }

            m_DynamicDependencies.Append( Dependency( n ) );
        }
    }
    return true;
}

// DoBuild
//------------------------------------------------------------------------------
/*virtual*/ Node::BuildResult CopyDirNode::DoBuild( Job * UNUSED( job ) )
{
    // consider ourselves to be as recent as the newest file
    uint64_t timeStamp = 0;
    const Dependency * const end = m_DynamicDependencies.End();
    for ( const Dependency * it = m_DynamicDependencies.Begin(); it != end; ++it )
    {
        CopyFileNode * cn = it->GetNode()->CastTo< CopyFileNode >();
        timeStamp = Math::Max< uint64_t >( timeStamp, cn->GetStamp() );
    }
    m_Stamp = timeStamp;

    return NODE_RESULT_OK;
}

// Load
//------------------------------------------------------------------------------
/*static*/ Node * CopyDirNode::Load( NodeGraph & nodeGraph, IOStream & stream )
{
    NODE_LOAD( AStackString<>, name );

    CopyDirNode * node = nodeGraph.CreateCopyDirNode( name );

    if ( node->Deserialize( nodeGraph, stream ) == false )
    {
        return nullptr;
    }
    return node;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void CopyDirNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    Node::Serialize( stream );
}

//------------------------------------------------------------------------------
