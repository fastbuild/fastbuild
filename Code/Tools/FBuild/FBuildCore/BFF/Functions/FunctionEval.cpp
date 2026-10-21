// FunctionEval
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionEval.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionEval::FunctionEval()
: Function( "Eval" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionEval::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionEval::NeedsHeader() const
{
    return true;
}

// NeedsBody
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionEval::NeedsBody() const
{
    return false;
}

//------------------------------------------------------------------------------
/*virtual*/ bool FunctionEval::ParseFunction(   NodeGraph & nodeGraph,
                                                BFFParser & parser,
                                                const BFFToken * functionNameStart,
                                                const BFFTokenRange & headerRange,
                                                const BFFTokenRange & bodyRange ) const
{
    (void)nodeGraph;
    (void)parser;
    (void)functionNameStart;
    (void)headerRange;
    (void)bodyRange;

    // we want 1 frame above this function
    BFFStackFrame * parentFrame = BFFStackFrame::GetCurrent()->GetParent();
    ASSERT( parentFrame );

    // parse it all out
    BFFTokenRange headerIter = headerRange;
    while ( headerIter.IsAtEnd() == false )
    {
        // Expect an existing source variable
        {
            const BFFToken * localVarToken = headerIter.GetCurrent();
            headerIter++;
            if ( localVarToken->IsVariable() == false )
            {
                Error::Error_1200_ExpectedVar( localVarToken, this );
                return false;
            }

            // Resolve the name of the local variable
            AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > localName;
            bool localParentScope = false;
            if ( BFFParser::ParseVariableName( localVarToken, localName, localParentScope ) == false )
            {
                return false;
            }

            // find variable
            const BFFVariable * v = nullptr;
            BFFStackFrame * const varFrame = ( localParentScope )
                ? BFFStackFrame::GetParentDeclaration( localName, parentFrame, v )
                : parentFrame;

            if ( false == localParentScope )
            {
                v = BFFStackFrame::GetVar( localName, nullptr );
            }

            if ( ( localParentScope && ( nullptr == varFrame ) ) || ( nullptr == v ) )
            {
                Error::Error_1009_UnknownVariable( localVarToken, this, localName );
                return false;
            }

            if ( PerformVariableSubstitutions( *localVarToken, v, varFrame ) == false )
            {
                return false;
            }
        }

        // skip optional separator
        if ( headerIter->IsOperator( "," ) )
        {
            headerIter++;
        }
    }

    return true;
}

// PerformVariableSubstitutions
//------------------------------------------------------------------------------
bool FunctionEval::PerformVariableSubstitutions( const BFFToken & iter,
                                                 const BFFVariable * v,
                                                 BFFStackFrame * frame ) const
{
    if ( v->IsString() )
    {
        AStackString< > expandedString;
        const BFFToken stringExpansion( iter.GetSourceFile(), iter.GetSourcePos(), BFFTokenType::String, v->GetString() );

        if ( BFFParser::PerformVariableSubstitutions( &stringExpansion, expandedString ) )
        {
            BFFStackFrame::SetVarString( v->GetName(), expandedString, frame );
            FLOG_INFO( "Registered <string> variable '%s' with value '%s'", v->GetName().Get(), expandedString.Get() );

            return true;
        }
    }
    else if ( v->IsArrayOfStrings() )
    {
        const Array< AString > & src = v->GetArrayOfStrings();
        const size_t srcSize = src.GetSize();

        Array< AString > expandedStrings( srcSize, false );
        expandedStrings.SetSize( srcSize );

        for ( size_t i = 0 ; i < srcSize ; i++ )
        {
            const BFFToken stringExpansion( iter.GetSourceFile(), iter.GetSourcePos(), BFFTokenType::String, src[i] );

            if ( false == BFFParser::PerformVariableSubstitutions( &stringExpansion, expandedStrings[i] ) )
            {
                return false;
            }
        }

        BFFStackFrame::SetVarArrayOfStrings( v->GetName(), expandedStrings, frame );
        FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", v->GetName().Get(), (uint32_t)expandedStrings.GetSize() );

        return true;
    }
    else if ( v->IsStruct() || v->IsArrayOfStructs() )
    {
        BFFStackFrame stackFrame;

        const Array< const BFFVariable * > & src = ( v->IsArrayOfStructs()
            ? v->GetArrayOfStructs()
            : v->GetStructMembers() );

        const size_t srcSize = src.GetSize();
        for ( size_t i = 0 ; i < srcSize ; i++ )
        {
            if ( false == PerformVariableSubstitutions( iter, src[i], &stackFrame ) )
            {
                return false;
            }
        }

        Array< BFFVariable * >& expandedVars = stackFrame.GetLocalVariables();

        if ( v->IsArrayOfStructs() )
        {
            // (PQU) #TODO this function signature will probably change in the future (following BFFStackFrame::SetVarStruct)
            // and we should then remove this glue code :
            StackArray<const BFFVariable *> constValues;
            constValues.Append( expandedVars );
            BFFStackFrame::SetVarArrayOfStructs( v->GetName(), constValues, frame );

            FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", v->GetName().Get(), (uint32_t)expandedVars.GetSize() );
        }
        else
        {
            ASSERT( v->IsStruct() );

            BFFStackFrame::SetVarStruct( v->GetName(), Move(expandedVars), frame );
            FLOG_INFO( "Registered <struct> variable '%s' with %u members", v->GetName().Get(), (uint32_t)expandedVars.GetSize() );
        }

        return true;
    }
    else
    {
        // simply copy variables without substitutions :
        BFFStackFrame::SetVar( v, frame );

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
