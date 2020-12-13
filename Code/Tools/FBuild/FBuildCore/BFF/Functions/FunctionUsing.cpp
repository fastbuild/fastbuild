// FunctionUsing
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionUsing.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionUsing::FunctionUsing()
: Function( "Using" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::NeedsHeader() const
{
    return true;
}

// NeedsBody
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::NeedsBody() const
{
    return false;
}

//------------------------------------------------------------------------------
/*virtual*/ bool FunctionUsing::ParseFunction( NodeGraph & /*nodeGraph*/,
                                               BFFParser & /*parser*/,
                                               const BFFToken * /*functionNameStart*/,
                                               const BFFTokenRange & headerRange,
                                               const BFFTokenRange & /*bodyRange*/ ) const
{
    ASSERT( headerRange.IsEmpty() == false );

    BFFTokenRange headerIter = headerRange;

    // a variable name?
    const BFFToken * varToken = headerIter.GetCurrent();
    headerIter++;
    if ( varToken->IsVariable() == false )
    {
        Error::Error_1007_ExpectedVariable( varToken, this );
        return false;
    }

    // we want 1 frame above this function
    BFFStackFrame * frame = BFFStackFrame::GetCurrent()->GetParent();
    ASSERT( frame );

    // find variable name
    AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > varName;
    bool parentScope = false;
    if ( BFFParser::ParseVariableName( varToken, varName, parentScope ) == false )
    {
        return false; // ParseVariableName will have emitted an error
    }

    // find variable
    const BFFVariable * v = nullptr;
    const BFFStackFrame * const varFrame = ( parentScope )
        ? BFFStackFrame::GetParentDeclaration( varName, frame, v )
        : nullptr;

    if ( false == parentScope )
    {
        v = BFFStackFrame::GetVar( varName, nullptr );
    }

    if ( ( parentScope && ( nullptr == varFrame ) ) || ( nullptr == v ) )
    {
        Error::Error_1009_UnknownVariable( varToken, this, varName );
        return false;
    }

    if ( v->IsStruct() == false )
    {
        Error::Error_1008_VariableOfWrongType( varToken, this,
                                                BFFVariable::VAR_STRUCT,
                                                v->GetType() );
        return false;
    }

    const BFFVariable * sameNameMember = nullptr;
    const Array< const BFFVariable * > & members = v->GetStructMembers();
    for ( const BFFVariable ** it = members.Begin();
            it != members.End();
            ++it )
    {
        const BFFVariable * member = ( *it );
        if ( ( sameNameMember == nullptr ) && ( parentScope == false ) && ( member->GetName() == v->GetName() ) )
        {
            // We have a struct member with the same name as the struct variable itself.
            // We have to delay putting it in the current scope because it may delete the struct variable and invalidate members array.
            sameNameMember = member;
            continue;
        }
        BFFStackFrame::SetVar( member, frame );
    }

    if ( sameNameMember != nullptr )
    {
        BFFStackFrame::SetVar( sameNameMember, frame );
    }

    if ( headerIter.IsAtEnd() == false )
    {
        // TODO:B Error for unexpected junk in header
    }

    return true;
}

//------------------------------------------------------------------------------
