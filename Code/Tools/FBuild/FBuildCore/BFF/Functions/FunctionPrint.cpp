// FunctionPrint
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionPrint.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenRange.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
FunctionPrint::FunctionPrint()
: Function( "Print" )
{
}

// AcceptsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::AcceptsHeader() const
{
    return true;
}

// NeedsHeader
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::NeedsHeader() const
{
    return true;
}

// NeedsBody
//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::NeedsBody() const
{
    return false;
}

//------------------------------------------------------------------------------
/*virtual*/ bool FunctionPrint::ParseFunction( NodeGraph & /*nodeGraph*/,
                                               BFFParser & /*parser*/,
                                               const BFFToken * /*functionNameStart*/,
                                               const BFFTokenRange & headerRange,
                                               const BFFTokenRange & /*bodyRange*/ ) const
{
    ASSERT( headerRange.IsEmpty() == false );

    // Grab token
    BFFTokenRange headerIter = headerRange;
    const BFFToken * varToken = headerIter.GetCurrent();
    headerIter++;

    if ( varToken->IsString() )
    {
        // perform variable substitutions
        AStackString< 1024 > tmp;
        if ( BFFParser::PerformVariableSubstitutions( varToken, tmp ) == false )
        {
            return false; // substitution will have emitted an error
        }
        tmp += '\n';

        if ( FBuild::Get().GetOptions().m_ShowPrintStatements )
        {
            FLOG_OUTPUT( tmp );
        }
    }
    else if ( varToken->IsVariable() )
    {
        // find variable name
        AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > varName;
        bool parentScope = false;
        if ( BFFParser::ParseVariableName( varToken, varName, parentScope ) == false )
        {
            return false; // ParseVariableName will have emitted an error
        }

        const BFFVariable * var = nullptr;
        const BFFStackFrame * const varFrame = ( parentScope )
            ? BFFStackFrame::GetParentDeclaration( varName, BFFStackFrame::GetCurrent()->GetParent(), var )
            : nullptr;

        if ( false == parentScope )
        {
            var = BFFStackFrame::GetVar( varName, nullptr );
        }

        if ( ( parentScope && ( nullptr == varFrame ) ) || ( nullptr == var ) )
        {
            Error::Error_1009_UnknownVariable( varToken, this, varName );
            return false;
        }

        // dump the contents
        if ( FBuild::Get().GetOptions().m_ShowPrintStatements )
        {
            PrintVarRecurse( *var, 0 );
        }
    }
    else
    {
        Error::Error_1001_MissingStringStartToken( varToken, this ); // TODO:C Better error message
        return false;
    }

    if ( headerIter.IsAtEnd() == false )
    {
        // TODO:B Error for unexpected junk in header
    }

    return true;
}

// PrintVarRecurse
//------------------------------------------------------------------------------
/*static*/ void FunctionPrint::PrintVarRecurse( const BFFVariable & var, uint32_t indent )
{
    AStackString<> indentStr;
    for ( uint32_t i=0; i<indent; ++i )
    {
        indentStr += "    ";
    }
    ++indent;
    FLOG_OUTPUT( "%s", indentStr.Get() );

    switch ( var.GetType() )
    {
        case BFFVariable::VAR_ANY: ASSERT( false ); break; // Something is terribly wrong
        case BFFVariable::VAR_STRING:
        {
            AStackString<> value( var.GetString() );
            value.Replace( "'", "^'" ); // escape single quotes
            FLOG_OUTPUT( "%s = '%s'\n", var.GetName().Get(), value.Get() );
            break;
        }
        case BFFVariable::VAR_BOOL:
        {
            FLOG_OUTPUT( "%s = %s\n", var.GetName().Get(), var.GetBool() ? BFF_KEYWORD_TRUE : BFF_KEYWORD_FALSE );
            break;
        }
        case BFFVariable::VAR_ARRAY_OF_STRINGS:
        {
            const Array<AString> & strings = var.GetArrayOfStrings();
            FLOG_OUTPUT( "%s = // ArrayOfStrings, size: %u\n%s{\n", var.GetName().Get(), (uint32_t)strings.GetSize(), indentStr.Get() );
            for ( const AString & string : strings )
            {
                AStackString<> value( string );
                value.Replace( "'", "^'" ); // escape single quotes
                FLOG_OUTPUT( "%s    '%s'\n", indentStr.Get(), value.Get() );
            }
            FLOG_OUTPUT( "%s}\n", indentStr.Get() );
            break;
        }
        case BFFVariable::VAR_INT:
        {
            FLOG_OUTPUT( "%s = %i\n", var.GetName().Get(), var.GetInt() );
            break;
        }
        case BFFVariable::VAR_STRUCT:
        {
            FLOG_OUTPUT( "%s = // Struct\n%s[\n", var.GetName().Get(), indentStr.Get() );
            for ( const BFFVariable * subVar : var.GetStructMembers() )
            {
                PrintVarRecurse( *subVar, indent );
            }
            FLOG_OUTPUT( "%s]\n", indentStr.Get() );
            break;
        }
        case BFFVariable::VAR_ARRAY_OF_STRUCTS:
        {
            const Array<const BFFVariable *> & structs = var.GetArrayOfStructs();
            FLOG_OUTPUT( "%s = // ArrayOfStructs, size: %u\n%s{\n", var.GetName().Get(), (uint32_t)structs.GetSize(), indentStr.Get() );
            for ( const BFFVariable * subVar : structs )
            {
                PrintVarRecurse( *subVar, indent );
            }
            FLOG_OUTPUT( "%s}\n", indentStr.Get() );
            break;
        }
        case BFFVariable::MAX_VAR_TYPES: ASSERT( false ); break; // Something is terribly wrong
    }
}

//------------------------------------------------------------------------------
