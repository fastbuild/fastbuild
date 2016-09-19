// FunctionPrint
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "FunctionPrint.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"

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
                                               const BFFIterator & functionNameStart,
                                               const BFFIterator * functionBodyStartToken,
                                               const BFFIterator * functionBodyStopToken,
                                               const BFFIterator * functionHeaderStartToken,
                                               const BFFIterator * functionHeaderStopToken ) const
{
    (void)functionNameStart;
    (void)functionBodyStartToken;
    (void)functionBodyStopToken;

    if ( functionHeaderStartToken && functionHeaderStopToken &&
         ( functionHeaderStartToken->GetDistTo( *functionHeaderStopToken ) >= 1 ) )
    {
        BFFIterator start( *functionHeaderStartToken );
        ASSERT( *start == BFFParser::BFF_FUNCTION_ARGS_OPEN );
        start++;
        start.SkipWhiteSpace();

        // a quoted string?
        const char c = *start;
        if ( ( c == '"' ) || ( c == '\'' ) )
        {
            // find end of string
            BFFIterator stop( start );
            stop.SkipString( c );
            ASSERT( stop.GetCurrent() <= functionHeaderStopToken->GetCurrent() ); // should not be in this function if strings are not validly terminated

            // perform variable substitutions
            AStackString< 1024 > tmp;

            start++; // skip past opening quote
            if ( BFFParser::PerformVariableSubstitutions( start, stop, tmp ) == false )
            {
                return false; // substitution will have emitted an error
            }
            tmp += '\n';

            FLOG_BUILD_DIRECT( tmp.Get() );
        }
        else if ( c == BFFParser::BFF_DECLARE_VAR_INTERNAL ||
                  c == BFFParser::BFF_DECLARE_VAR_PARENT )
        {
            // find end of var name
            BFFIterator stop( start );
            AStackString< BFFParser::MAX_VARIABLE_NAME_LENGTH > varName;
            bool parentScope = false;
            if ( BFFParser::ParseVariableName( stop, varName, parentScope ) == false )
            {
                return false;
            }

            ASSERT( stop.GetCurrent() <= functionHeaderStopToken->GetCurrent() ); // should not be in this function if strings are not validly terminated

            const BFFVariable * var = nullptr;
            BFFStackFrame * const varFrame = ( parentScope )
                ? BFFStackFrame::GetParentDeclaration( varName, BFFStackFrame::GetCurrent()->GetParent(), var )
                : nullptr;

            if ( false == parentScope )
            {
                var = BFFStackFrame::GetVar( varName, nullptr );
            }

            if ( ( parentScope && ( nullptr == varFrame ) ) || ( nullptr == var ) )
            {
                Error::Error_1009_UnknownVariable( start, this );
                return false;
            }

            // dump the contents
            PrintVarRecurse( *var, 0 );
        }
        else
        {
            Error::Error_1001_MissingStringStartToken( start, this );
            return false;
        }
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
    FLOG_BUILD( indentStr.Get() );

    switch ( var.GetType() )
    {
        case BFFVariable::VAR_ANY: ASSERT( false ); break; // Something is terribly wrong
        case BFFVariable::VAR_STRING:
        {
            AStackString<> value( var.GetString() );
            value.Replace( "'", "^'" ); // escape single quotes
            FLOG_BUILD( "%s = '%s'\n", var.GetName().Get(), value.Get() );
            break;
        }
        case BFFVariable::VAR_BOOL:
        {
            FLOG_BUILD( "%s = %s\n", var.GetName().Get(), var.GetBool() ? "true" : "false" );
            break;
        }
        case BFFVariable::VAR_ARRAY_OF_STRINGS:
        {
            const auto & strings = var.GetArrayOfStrings();
            FLOG_BUILD( "%s = // ArrayOfStrings, size: %u\n%s{\n", var.GetName().Get(), (uint32_t)strings.GetSize(), indentStr.Get() );
            for ( const AString & string : strings )
            {
                AStackString<> value( string );
                value.Replace( "'", "^'" ); // escape single quotes
                FLOG_BUILD( "%s    '%s'\n", indentStr.Get(), value.Get() );
            }
            FLOG_BUILD( "%s}\n", indentStr.Get() );
            break;
        }
        case BFFVariable::VAR_INT:
        {
            FLOG_BUILD( "%s = %i\n", var.GetName().Get(), var.GetInt() );
            break;
        }
        case BFFVariable::VAR_STRUCT:
        {
            FLOG_BUILD( "%s = // Struct\n%s[\n", var.GetName().Get(), indentStr.Get() );
            for ( const BFFVariable * subVar : var.GetStructMembers() )
            {
                PrintVarRecurse( *subVar, indent );
            }
            FLOG_BUILD( "%s]\n", indentStr.Get() );
            break;
        }
        case BFFVariable::VAR_ARRAY_OF_STRUCTS:
        {
            const auto & structs = var.GetArrayOfStructs();
            FLOG_BUILD( "%s = // ArrayOfStructs, size: %u\n%s{\n", var.GetName().Get(), (uint32_t)structs.GetSize(), indentStr.Get() );
            for ( const BFFVariable * subVar : structs )
            {
                PrintVarRecurse( *subVar, indent );
            }
            FLOG_BUILD( "%s}\n", indentStr.Get() );
            break;
        }
        case BFFVariable::MAX_VAR_TYPES: ASSERT( false ); break; // Something is terribly wrong
    }
}

//------------------------------------------------------------------------------
