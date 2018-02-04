// FunctionDLL
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionDLL.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/Graph/DLLNode.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionDLL::FunctionDLL()
: FunctionExecutable()
{
    // override name set by FunctionExecutable base class
    m_Name =  "DLL";
}

// Commit
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionDLL::Commit( NodeGraph & nodeGraph, const BFFIterator & funcStartIter ) const
{
    AStackString<> name;
    if ( GetNameForNode( nodeGraph, funcStartIter, DLLNode::GetReflectionInfoS(), name ) == false )
    {
        return false;
    }
    if ( nodeGraph.FindNode( name ) )
    {
        Error::Error_1100_AlreadyDefined( funcStartIter, this, name );
        return false;
    }

    DLLNode * dllNode = nodeGraph.CreateDLLNode( name );

    if ( !PopulateProperties( nodeGraph, funcStartIter, dllNode ) )
    {
        return false;
    }

    if ( !dllNode->Initialize( nodeGraph, funcStartIter, this ) )
    {
        return false;
    }

    return ProcessAlias( nodeGraph, funcStartIter, dllNode );
}

//------------------------------------------------------------------------------
