// RemoveDirNode.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "RemoveDirNode.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/DirectoryListNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Env/Env.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
RemoveDirNode::RemoveDirNode( const AString & name,
                              Dependencies & staticDeps,
                              const Dependencies & preBuildDeps )
: Node( name, Node::REMOVE_DIR_NODE, Node::FLAG_NONE )
{
    m_StaticDependencies.Append( staticDeps );
    m_PreBuildDependencies = preBuildDeps;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
RemoveDirNode::~RemoveDirNode()
{
}

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
/*static*/ Node * RemoveDirNode::Load( IOStream & stream )
{
    NODE_LOAD( AStackString<>,  name);
    NODE_LOAD_DEPS( 4,          staticDeps );
    NODE_LOAD_DEPS( 0,          preBuildDeps );

    NodeGraph & ng = FBuild::Get().GetDependencyGraph();
    RemoveDirNode * n = ng.CreateRemoveDirNode( name, staticDeps, preBuildDeps );
    ASSERT( n );
    return n;
}

// Save
//------------------------------------------------------------------------------
/*virtual*/ void RemoveDirNode::Save( IOStream & stream ) const
{
    NODE_SAVE( m_Name );
    NODE_SAVE_DEPS( m_StaticDependencies );
    NODE_SAVE_DEPS( m_PreBuildDependencies );
}

//------------------------------------------------------------------------------
