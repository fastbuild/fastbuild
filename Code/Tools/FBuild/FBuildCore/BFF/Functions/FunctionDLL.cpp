// FunctionDLL
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionDLL.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionDLL::FunctionDLL()
: FunctionExecutable()
{
    // override name set by FunctionExecutable base class
    m_Name =  "DLL";
}

//------------------------------------------------------------------------------
