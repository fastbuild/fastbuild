// FunctionPrint
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FunctionPrint.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFKeywords.h"
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
        if ( start.IsAtString() )
        {
            // find end of string
            BFFIterator stop( start );
            stop.SkipString();
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
        else if ( c == '/' )
        {
            // find end of pattern
            BFFIterator stop( start );
            stop.SkipString( c );
            ASSERT( stop.GetCurrent() < functionHeaderStopToken->GetCurrent() ); // should not be in this function if strings are not validly terminated
        
            // perform variable substitutions
            AStackString< 1024 > pat;
        
            start++; // skip past opening quote
            if ( BFFParser::PerformVariableSubstitutions( start, stop, pat ) == false )
            {
                return false; // substitution will have emitted an error
            }

            stop++;
            stop.SkipWhiteSpaceAndComments();
            
            // Look at the options.
            bool iterateParents = false;
            bool shouldRecurse = false;
        
            for ( BFFIterator iterOpt = stop; stop.GetCurrent() < functionHeaderStopToken->GetCurrent(); )
            {
                switch ( *iterOpt )
                {
                    case 'P':
                        iterateParents = true;
                        break;
                    case '+':
                        shouldRecurse = true;
                        break;
                    default:
                    {
                        AStackString<32> err;
                        err.Format("Print pattern option %c", *iterOpt );
                        Error::Error_1030_UnknownDirective(iterOpt, err);
                        return false;
                    }
                }
                stop++;
                stop.SkipWhiteSpaceAndComments();
            }
        
            PrintStackVars( pat, iterateParents, shouldRecurse);
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
                Error::Error_1009_UnknownVariable( start, this, varName );
                return false;
            }

            // dump the contents
            PrintVarRecurse( *var, 0, 0xFFFFFFFF );
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
/*static*/ void FunctionPrint::PrintVarRecurse( const BFFVariable & var, uint32_t indent, uint32_t maxIndent )
{
    AStackString<> indentStr;
    for ( uint32_t i=0; i<indent; ++i )
    {
        indentStr += "    ";
    }
    ++indent;
    FLOG_BUILD( "%s", indentStr.Get() );

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
            FLOG_BUILD( "%s = %s\n", var.GetName().Get(), var.GetBool() ? BFF_KEYWORD_TRUE : BFF_KEYWORD_FALSE );
            break;
        }
        case BFFVariable::VAR_ARRAY_OF_STRINGS:
        {
            const auto & strings = var.GetArrayOfStrings();
            FLOG_BUILD( "%s = // ArrayOfStrings, size: %u\n", var.GetName().Get(), (uint32_t)strings.GetSize() );
            if ( maxIndent >= indent )
            {
                FLOG_BUILD( "%s{\n", indentStr.Get() );
                for ( const AString & string : strings )
                {
                    AStackString<> value( string );
                    value.Replace( "'", "^'" ); // escape single quotes
                    FLOG_BUILD( "%s    '%s'\n", indentStr.Get(), value.Get() );
                }
                FLOG_BUILD( "%s}\n", indentStr.Get() );
            }
            break;
        }
        case BFFVariable::VAR_INT:
        {
            FLOG_BUILD( "%s = %i\n", var.GetName().Get(), var.GetInt() );
            break;
        }
        case BFFVariable::VAR_STRUCT:
        {
            FLOG_BUILD( "%s = // Struct\n", var.GetName().Get() );
            if ( maxIndent >= indent )
            {
                FLOG_BUILD( "%s[\n", indentStr.Get() );
                for ( const BFFVariable * subVar : var.GetStructMembers() )
                {
                    PrintVarRecurse( *subVar, indent, maxIndent );
                }
                FLOG_BUILD( "%s]\n", indentStr.Get() );
            }
            break;
        }
        case BFFVariable::VAR_ARRAY_OF_STRUCTS:
        {
            const auto & structs = var.GetArrayOfStructs();
            FLOG_BUILD( "%s = // ArrayOfStructs, size: %u\n", var.GetName().Get(), (uint32_t)structs.GetSize() );
            if ( maxIndent >= indent )
            {
                FLOG_BUILD( "%s{\n", indentStr.Get() );
                for ( const BFFVariable * subVar : structs )
                {
                    PrintVarRecurse( *subVar, indent, maxIndent );
                }
                FLOG_BUILD( "%s}\n", indentStr.Get() );
            }
            break;
        }
        case BFFVariable::MAX_VAR_TYPES: ASSERT( false ); break; // Something is terribly wrong
    }
}


// PrintVarRecurse
//------------------------------------------------------------------------------
/*static*/ void FunctionPrint::PrintStackVars( const AString& pattern, bool iterateParents, bool shouldRecurse )
{
    // Build an array of BFFStackFrame so it can be iterated in reverse order as 
    // the frame stack (global variables get printed first)
    Array< const BFFStackFrame* > aFrames(/*initialCapacity=*/10, /*resizable=*/true );
    {
        BFFStackFrame *pFrame = BFFStackFrame::GetCurrent();
        if ( pFrame->GetParent() )
        {
            // Print gets a stack frame even though it doesn't have {}, and
            // that frame isn't interesting to the caller.  So iterate past it.
            pFrame = pFrame->GetParent();
        }
        do
        {
            aFrames.Append( pFrame );
            pFrame = pFrame->GetParent();
        } while ( pFrame != nullptr && iterateParents );
    }
    const int iNumFrames = static_cast<int>(aFrames.GetSize());

    const int iMaxDepth = shouldRecurse ? 0xFFFFFFFF : 0;
    const bool bPatternMatchesAll = (pattern == "*");
    for ( int iFrame = iNumFrames - 1; iFrame >= 0; --iFrame )
    {
        const BFFStackFrame* const pFrame = aFrames[ iFrame ];
        const int iDepth = iNumFrames - iFrame - 1;
        const Array< const BFFVariable* >& rVariablesInFrame = pFrame->GetLocalVariables();
        for ( int iVar = 0; iVar < rVariablesInFrame.GetSize(); ++iVar )
        {
            const BFFVariable* const pVar = rVariablesInFrame[ iVar ];
            if ( bPatternMatchesAll || pVar->GetName().Matches( pattern.Get() ) )
            {
                PrintVarRecurse(*pVar, iDepth, iMaxDepth);
            }
        }
    }

}

//------------------------------------------------------------------------------
