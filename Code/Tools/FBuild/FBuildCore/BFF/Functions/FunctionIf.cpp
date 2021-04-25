// FunctionIf
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionIf.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFBooleanExpParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionIf::FunctionIf()
: Function( "If" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionIf::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionIf::NeedsHeader() const
{
    return true;
}

// ParseFunction
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionIf::ParseFunction( NodeGraph & /*nodeGraph*/,
                                            BFFParser & parser,
                                            const BFFToken * /*functionNameStart*/,
                                            const BFFTokenRange & headerRange,
                                            const BFFTokenRange & bodyRange ) const
{

    // Iterate the args
    BFFTokenRange header( headerRange );

    bool headerResult;    
    if ( !BFFBooleanExpParser::Parse( this, header, headerResult ) )
    {
        return false;
    }
    
    if ( !headerResult )
    {
        return true;
    }

    BFFTokenRange body( bodyRange );
    return parser.Parse( body );
}

//------------------------------------------------------------------------------
