// FunctionTest
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionTextFile.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/Graph/TextFileNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionTextFile::FunctionTextFile()
: Function( "TextFile" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionTextFile::AcceptsHeader() const
{
    return true;
}

// CreateNode
//------------------------------------------------------------------------------
/*virtual*/ Node * FunctionTextFile::CreateNode() const
{
    return FNEW( TextFileNode );
}

//------------------------------------------------------------------------------
