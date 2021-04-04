// CompilerDriver_CodeWarriorWii.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "CompilerDriver_CodeWarriorWii.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Helpers/Args.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_CodeWarriorWii::CompilerDriver_CodeWarriorWii() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
CompilerDriver_CodeWarriorWii::~CompilerDriver_CodeWarriorWii() = default;

// ProcessArg_PreprocessorOnly
//------------------------------------------------------------------------------
/*virtual*/ bool CompilerDriver_CodeWarriorWii::ProcessArg_PreprocessorOnly( const AString & token,
                                                                             size_t & index,
                                                                             const AString & /*nextToken*/,
                                                                             Args & /*outFullArgs*/ ) const
{
    // Remove output arg
    if ( StripTokenWithArg( "-o", token, index ) )
    {
        return true;
    }

    // Remove "compile only" flag
    if ( StripToken( "-c", token ) )
    {
        return true;
    }

    return false;
}

// AddAdditionalArgs_Preprocessor
//------------------------------------------------------------------------------
/*virtual*/ void CompilerDriver_CodeWarriorWii::AddAdditionalArgs_Preprocessor( Args & outFullArgs ) const
{
    outFullArgs += "-E"; // run pre-processor only
}

//------------------------------------------------------------------------------
