// FunctionLibrary
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionLibrary.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/LibraryNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionLibrary::FunctionLibrary()
: FunctionObjectList()
{
    m_Name = "Library";
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::NeedsHeader() const
{
    return false;
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionLibrary::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name;
    if ( GetNameForNode( nodeGraph, funcStartIter, LibraryNode::GetReflectionInfoS(), name ) == false )
    {
        return false;
    }

    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }
    LibraryNode * libraryNode = nodeGraph.CreateLibraryNode( name );
    
    if ( !PopulateProperties( nodeGraph, funcStartIter, libraryNode ) )
    {
        return false;
    }
    
    if ( !libraryNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    // handle alias creation
    return ProcessAlias( nodeGraph, funcStartIter, libraryNode );
}

//------------------------------------------------------------------------------
