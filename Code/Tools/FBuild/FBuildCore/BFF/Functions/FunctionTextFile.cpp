// FunctionTextFile
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionTextFile.h"
#include "Tools/FBuild/FBuildCore/Graph/TextFileNode.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionTextFile::FunctionTextFile()
    : Function( "TextFile" )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
/*virtual*/ FunctionTextFile::~FunctionTextFile() = default;

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
