// BFFParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "BFFParser.h"
#include "BFFIterator.h"
#include "BFFMacros.h"
#include "BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"

// Core
#include "Core/Containers/AutoPtr.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"
#include "Core/Tracing/Tracing.h"

#include <stdio.h>

// Static Data
//------------------------------------------------------------------------------
/*static*/ uint32_t BFFParser::s_Depth( 0 );
/*static*/ uint32_t BFFParser::s_IfDepth( 0 );

// CONSTRUCTOR
//------------------------------------------------------------------------------
BFFParser::BFFParser()
: m_SeenAVariable( false )
, m_LastVarFrame( nullptr )
{
	++s_Depth;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
BFFParser::~BFFParser()
{
	ASSERT( s_Depth > 0 );
	--s_Depth;
}

// Parse
//------------------------------------------------------------------------------
bool BFFParser::Parse( const char * dataWithSentinel, 
					   uint32_t sizeExcludingSentinel, 
					   const char * fileName,
					   uint64_t fileTimeStamp,
					   bool pushStackFrame )
{
	// data should be 1 bytes larger than size, with a sentinel
	ASSERT( dataWithSentinel[ sizeExcludingSentinel ] == '\000' );

	if ( FBuild::IsValid() ) // cope with null for unit tests
	{
		// NOTE: filename may or may not be clean already - ok to do twice
		AStackString<> fileNameClean;
		NodeGraph::CleanPath( AStackString<>( fileName ), fileNameClean );
		FBuild::Get().GetDependencyGraph().AddUsedFile( fileNameClean, fileTimeStamp );
	}

	// parse it
	BFFIterator iter( dataWithSentinel, sizeExcludingSentinel, fileName, fileTimeStamp );
	if ( pushStackFrame )
	{
		BFFStackFrame stackFrame;
		return Parse( iter );
	}
	else
	{
		return Parse( iter );
	}
}

// Parse
//------------------------------------------------------------------------------
bool BFFParser::Parse( BFFIterator & iter )
{
	for (;;)
	{
		iter.SkipWhiteSpace();

		// is this a comment?
		if ( iter.IsAtComment() )
		{
			iter.SkipComment();
			continue;
		}

		const char c = *iter;
		switch ( c )
		{
			case BFF_DECLARE_VAR_INTERNAL:
			case BFF_DECLARE_VAR_PARENT:
			{
				if ( ParseNamedVariableDeclaration( iter ) == false )
				{
					return false;
				}
				continue;
			}
			case BFF_VARIABLE_CONCATENATION:
			case BFF_VARIABLE_SUBTRACTION:
			{
				// concatenation to last used variable
				if ( ParseUnnamedVariableModification( iter ) == false )
				{
					return false;
				}
				continue;
			}
			case BFF_SCOPE_OPEN:
			{
				// start an unnamed scope
				if ( ParseUnnamedScope( iter ) == false )
				{
					return false;
				}
				continue;
			}
			case BFF_PREPROCESSOR_START:
			{
				if ( ParsePreprocessorDirective( iter ) == false )
				{
					return false;
				}
				continue;
			}
			default:
			{
				if ( iter.IsAtValidFunctionNameCharacter() )
				{
					if ( ParseFunction( iter ) == false )
					{
						return false;
					}
					continue;
				}
			}
		}

		iter.SkipWhiteSpace();
		if ( iter.IsAtEnd() == false )
		{
			Error::Error_1010_UnknownConstruct( iter );
			return false;
		}

		break;  // cleanly hit end of file
	}

	return true;
}

// ParseNamedVariableName
//------------------------------------------------------------------------------
/*static*/ bool BFFParser::ParseVariableName( BFFIterator & iter, AString & name, bool & parentScope )
{
	// skip over the declaration symbol
	ASSERT( *iter == BFF_DECLARE_VAR_INTERNAL ||
			*iter == BFF_DECLARE_VAR_PARENT );

	parentScope = ( *iter == BFF_DECLARE_VAR_PARENT );

	const BFFIterator varNameStart = iter; // include type token in var name
	iter++;

	// make sure we haven't hit the end of the file
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	if ( *iter == '\'' || *iter == '"' )
	{
		// parse the string
		const BFFIterator openToken = iter;
		iter.SkipString( *openToken );
		if ( *iter != *openToken )
		{
			Error::Error_1002_MatchingClosingTokenNotFound( openToken, nullptr, *openToken );
			return false;
		}
		BFFIterator stringStart = openToken;
		stringStart++;

		// unescape and subsitute embedded variables
		AStackString< 256 > value;
		if ( PerformVariableSubstitutions( stringStart, iter, value ) == false )
		{
			return false;
		}
		iter++; // skip close token

		BFFIterator varNameIter( value.Get(), value.GetLength(), iter.GetFileName().Get(), iter.GetFileTimeStamp() );

		// sanity check it is a sensible length
		if ( value.GetLength() + 1/* '.' will be added */  > MAX_VARIABLE_NAME_LENGTH )
		{
			Error::Error_1014_VariableNameIsTooLong( varNameIter, (uint32_t)value.GetLength(), (uint32_t)MAX_VARIABLE_NAME_LENGTH );
			return false;
		}

		// sanity check it is a valid variable name
		while ( varNameIter.IsAtEnd() == false )
		{
			if ( varNameIter.IsAtValidVariableNameCharacter() == false )
			{
				Error::Error_1013_UnexpectedCharInVariableName( varNameIter, nullptr );
				return false;
			}
			varNameIter++;
		}

		// append '.' to variable name
		name = ".";
		name.Append( value );
	}
	else
	{
		// make sure immediately after the symbol starts a variable name
		if ( iter.IsAtValidVariableNameCharacter() == false )
		{
			Error::Error_1013_UnexpectedCharInVariableName( iter, nullptr );
			return false;
		}

		// find the end of the variable name
		iter.SkipVariableName();
		const BFFIterator varNameEnd = iter;

		// sanity check it is a sensible length
		size_t varNameLen = varNameStart.GetDistTo( varNameEnd );
		if ( varNameLen > MAX_VARIABLE_NAME_LENGTH )
		{
			Error::Error_1014_VariableNameIsTooLong( iter, (uint32_t)varNameLen, (uint32_t)MAX_VARIABLE_NAME_LENGTH );
			return false;
		}

		// store variable name
		name.Assign( varNameStart.GetCurrent(), varNameEnd.GetCurrent() );
	}

	ASSERT( name.GetLength() > 0 );
	if ( parentScope )
	{
		// exchange '^' with '.'
		ASSERT( BFF_DECLARE_VAR_PARENT == name[0] );
		name[0] = BFF_DECLARE_VAR_INTERNAL;
	}

	return true;
}

// ParseUnnamedVariableModification
//------------------------------------------------------------------------------
bool BFFParser::ParseUnnamedVariableModification( BFFIterator & iter )
{
	ASSERT( ( *iter == BFF_VARIABLE_CONCATENATION ) || 
			( *iter == BFF_VARIABLE_SUBTRACTION ) );

	// have we assigned a variable before?
	if ( m_SeenAVariable == false )
	{
		Error::Error_1011_UnnamedModifcationMustFollowAssignment( iter );
		return false;
	}

	return ParseVariableDeclaration( iter, m_LastVarName, m_LastVarFrame );
}

// ParseNamedVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseNamedVariableDeclaration( BFFIterator & iter )
{
	const BFFIterator varNameStart( iter );

	bool parentScope = false;
	if ( ParseVariableName( iter, m_LastVarName, parentScope ) == false )
	{
		return false; // ParseVariableName() would have display an error
	}

	// find the start of the assignment
	iter.SkipWhiteSpaceAndComments();
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	// check if points to a previous declaration in a parent scope
	const BFFVariable * parentVar = nullptr;
	m_LastVarFrame = ( parentScope )
		? BFFStackFrame::GetParentDeclaration( m_LastVarName, nullptr, parentVar )
		: nullptr;

    if ( parentScope )
    {
    	// check if a parent definition exists
    	if ( nullptr == m_LastVarFrame )
	    {
	    	Error::Error_1009_UnknownVariable( varNameStart, nullptr );
	    	return false;
	    }

	    ASSERT( nullptr != parentVar );

	    // check if the parent definition is frozen
    	if ( parentVar->Frozen() )
    	{
    		Error::Error_1060_CantModifyFrozenVar( varNameStart, nullptr, parentVar );
	    	return false;
    	}
    }

	return ParseVariableDeclaration( iter, m_LastVarName, m_LastVarFrame );
}

// ParseVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseVariableDeclaration( BFFIterator & iter, const AString & varName, BFFStackFrame * frame )
{
	m_SeenAVariable = true;

	// look for an appropriate operator
	BFFIterator operatorIter( iter );
	bool modification = false;
	if ( *iter == BFF_VARIABLE_ASSIGNMENT )
	{
		// assignment
	}
	else if ( ( *iter == BFF_VARIABLE_CONCATENATION ) ||
			  ( *iter == BFF_VARIABLE_SUBTRACTION ) )
	{
		// concatenation
		modification = true;
	}
	else
	{
		Error::Error_1016_UnexepectedCharFollowingVariableName( iter );
		return false;
	}

	// skip the assignment symbol and whitespace
	iter++;
	iter.SkipWhiteSpaceAndComments();
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	char openToken = *iter;
	char closeToken = 0;
	bool ok = false;
	if ( ( openToken == '"' ) || ( openToken == '\'' ) )
	{
		closeToken = openToken;
		ok = true;
	}
	else if ( openToken == BFF_SCOPE_OPEN )
	{
		closeToken = BFF_SCOPE_CLOSE;
		ok = true;
	}
	else if ( openToken == BFF_STRUCT_OPEN )
	{
		closeToken = BFF_STRUCT_CLOSE;
		ok = true;
	}
	else if ( ( openToken >= '0' ) && ( openToken <= '9' ) )
	{
		if ( modification )
		{
			Error::Error_1027_CannotModify( operatorIter, varName, BFFVariable::VAR_ANY, BFFVariable::VAR_INT );
			return false;
		}

		// integer value?
		BFFIterator startIntValue( iter );
		while ( iter.IsAtEnd() == false )
		{
			iter++;
			if ( ( *iter < '0' ) || ( *iter > '9' ) )
			{
				break; // end of integer
			}
		}
		if ( startIntValue.GetDistTo( iter ) > 10 )
		{
			Error::Error_1018_IntegerValueCouldNotBeParsed( startIntValue );
			return false;
		}
		AStackString<> intAsString( startIntValue.GetCurrent(), iter.GetCurrent() );
		int i = 0;
		if ( sscanf( intAsString.Get(), "%i", &i ) != 1 )
		{
			Error::Error_1018_IntegerValueCouldNotBeParsed( startIntValue );
			return false;
		}
		return StoreVariableInt( varName, i, frame );
	}
	else if ( ( *iter == 't' ) || ( *iter == 'f' ) )
	{
		// might be 'true' or 'false'
		BFFIterator startBoolValue( iter );
		if ( iter.ParseToNext( 'e' ) == true )
		{
			iter++;
			if ( ( startBoolValue.GetDistTo( iter ) <= 5 ) )
			{
				AStackString<8> value( startBoolValue.GetCurrent(), iter.GetCurrent() );
				if ( value == "true" )
				{
					if ( modification )
					{
						Error::Error_1027_CannotModify( operatorIter, varName, BFFVariable::VAR_ANY, BFFVariable::VAR_BOOL );
						return false;
					}
					return StoreVariableBool( varName, true, frame );
				}
				else if ( value == "false" )
				{
					if ( modification )
					{
						Error::Error_1027_CannotModify( operatorIter, varName, BFFVariable::VAR_ANY, BFFVariable::VAR_BOOL );
						return false;
					}
					return StoreVariableBool( varName, false, frame );
				}
			}
		}

		// not a valid bool value
	}
	else if ( *iter == BFF_DECLARE_VAR_INTERNAL ||
			  *iter == BFF_DECLARE_VAR_PARENT )
	{
		return StoreVariableToVariable( varName, iter, operatorIter, frame );
	}

	if ( !ok )
	{
		Error::Error_1017_UnexepectedCharInVariableValue( iter );
		return false;
	}

	bool result = false;

	// find closing token
	BFFIterator openTokenPos( iter );
	BFFIterator openTokenForError( iter ); // take note of opening token pos for possible error
	openTokenPos++; // more start iter to first char of value
	if ( openToken == BFF_SCOPE_OPEN )
	{
		if ( iter.ParseToMatchingBrace( openToken, closeToken ) )
		{
			result = StoreVariableArray( varName, openTokenPos, iter, operatorIter, frame );
		}
		else
		{
			Error::Error_1002_MatchingClosingTokenNotFound( openTokenForError, nullptr, closeToken );
		}
	}
	else if ( openToken == BFF_STRUCT_OPEN )
	{
		if ( iter.ParseToMatchingBrace( openToken, closeToken ) )
		{
			result = StoreVariableStruct( varName, openTokenPos, iter, operatorIter, frame );
		}
		else
		{
			Error::Error_1002_MatchingClosingTokenNotFound( openTokenForError, nullptr, closeToken );
		}
	}
	else
	{
		ASSERT( ( openToken == '\'' ) || ( openToken == '"' ) );
		iter.SkipString( closeToken );
		if ( *iter == closeToken )
		{
			result = StoreVariableString( varName, openTokenPos, iter, operatorIter, frame );
		}
		else		
		{
			Error::Error_1002_MatchingClosingTokenNotFound( openTokenForError, nullptr, closeToken );
		}
	}

	if ( result )
	{
		iter++; // skip over the end token
	}

	// StoreVariable will have emitted an error if there was one
	return result;
}

// ParseFunction
//------------------------------------------------------------------------------
bool BFFParser::ParseFunction( BFFIterator & iter )
{
	ASSERT( iter.IsAtValidFunctionNameCharacter() );

	// for variables to be used by this function
	BFFStackFrame stackFrame;

	BFFIterator functionNameStart( iter );
	iter.SkipFunctionName();
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	// check length
	if ( functionNameStart.GetDistTo( iter ) > MAX_FUNCTION_NAME_LENGTH )
	{
		// if it's too long, then it can't be a valid function
		Error::Error_1015_UnknownFunction( functionNameStart );
		return false;
	}

	// store function name
	AStackString<MAX_FUNCTION_NAME_LENGTH> functionName( functionNameStart.GetCurrent(), iter.GetCurrent() );
	const Function * func = Function::Find( functionName );
	if ( func == nullptr )
	{
		Error::Error_1015_UnknownFunction( functionNameStart );
		return false;
	}
	iter.SkipWhiteSpace();

	if ( func->IsUnique() && func->GetSeen() )
	{
		Error::Error_1020_FunctionCanOnlyBeInvokedOnce( functionNameStart, func );
		return false;
	}
	func->SetSeen();

	FLOG_INFO( "Function call '%s'", functionName.Get() );

	// header, or body?
	bool hasHeader = false;
	BFFIterator functionArgsStartToken( iter );
	BFFIterator functionArgsStopToken( iter );
	if ( *iter == BFF_FUNCTION_ARGS_OPEN )
	{
		// can this function accept a header?
		if ( func->AcceptsHeader() == false )
		{
			Error::Error_1021_UnexpectedHeaderForFunction( iter, func );
			return false;
		}

		// args
		if ( iter.ParseToMatchingBrace( BFF_FUNCTION_ARGS_OPEN, BFF_FUNCTION_ARGS_CLOSE ) == false )
		{
			Error::Error_1022_MissingFunctionHeaderCloseToken( functionArgsStartToken, func );
			return false;
		}
		functionArgsStopToken = iter;
		hasHeader = true;
		iter++; // skip over closing token	
		iter.SkipWhiteSpaceAndComments();
	}

	if ( func->NeedsHeader() && ( hasHeader == false ) )
	{
		Error::Error_1023_FunctionRequiresAHeader( iter, func );
		return false;
	}

	// some functions have no body
	bool hasBody = false;

	BFFIterator functionBodyStartToken( iter );
	BFFIterator functionBodyStopToken( iter );
	if ( func->NeedsBody() )
	{
		// find body
		if ( *iter != BFF_SCOPE_OPEN )
		{
			Error::Error_1024_FunctionRequiresABody( functionNameStart, func );
			return false;
		}

		if ( iter.ParseToMatchingBrace( BFF_SCOPE_OPEN, BFF_SCOPE_CLOSE ) == false )
		{
			Error::Error_1025_MissingScopeCloseToken( functionBodyStartToken, func );
			return false;
		}

		functionBodyStopToken = iter;
		iter++;
		hasBody = true;
	}

	return func->ParseFunction( functionNameStart,
								hasBody ? &functionBodyStartToken : nullptr, 
								hasBody ? &functionBodyStopToken : nullptr,
								hasHeader ? &functionArgsStartToken : nullptr,
								hasHeader ? &functionArgsStopToken : nullptr );}

// ParseUnnamedScope
//------------------------------------------------------------------------------
bool BFFParser::ParseUnnamedScope( BFFIterator & iter )
{
	// find the matching bracket
	BFFIterator scopeStart( iter );
	if ( iter.ParseToMatchingBrace( BFF_SCOPE_OPEN, BFF_SCOPE_CLOSE ) == false )
	{
		Error::Error_1025_MissingScopeCloseToken( scopeStart, nullptr );
		return false;
	}

	// create stack for scope
	BFFStackFrame stackFrame;

	// parse the scoped part
	BFFParser subParser;
	BFFIterator subIter( scopeStart );
	subIter++; // skip opening token
	subIter.SetMax( iter.GetCurrent() ); // limit to closing token
	if ( subParser.Parse( subIter ) == false )
	{
		return false;
	}

	iter++; // skip closing }

	return true;
}

// ParsePreprocessorDirective
//------------------------------------------------------------------------------
bool BFFParser::ParsePreprocessorDirective( BFFIterator & iter )
{
	const BFFIterator directiveStart( iter );

	// skip directive start token
	ASSERT( *iter == BFF_PREPROCESSOR_START );
	iter++;

	// allow whitepace before directive name
	iter.SkipWhiteSpace();

	// start of directive name
	BFFIterator directiveStartIter( iter );

	// find end of directive
	while ( iter.IsAtValidDirectiveNameCharacter() )
	{
		iter++;
	}
	BFFIterator directiveEndIter( iter );

	iter.SkipWhiteSpace();

	// determine directive
	AStackString< MAX_DIRECTIVE_NAME_LENGTH > directive( directiveStartIter.GetCurrent(), directiveEndIter.GetCurrent() );
	if ( directive == "include" )
	{
		return ParseIncludeDirective( iter );
	}
	else if ( directive == "once" )
	{
		FBuild::Get().GetDependencyGraph().SetCurrentFileAsOneUse();
		return true;
	}
	else if ( directive == "define" )
	{
		return ParseDefineDirective( iter );
	}
	else if ( directive == "undef" )
	{
		return ParseUndefDirective( iter );
	}
	else if ( directive == "if" )
	{
		return ParseIfDirective( directiveStart, iter );
	}
	else if ( directive == "endif" )
	{
		return ParseEndIfDirective( directiveStartIter );
	}
	else if ( directive == "import" )
	{
		return ParseImportDirective( directiveStart, iter );
	}

	// unknown
	Error::Error_1030_UnknownDirective( directiveStartIter, directive );
	return false;
}

// ParseIncludeDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseIncludeDirective( BFFIterator & iter )
{
	// Sanity check include depth to detect cyclic includes
	if ( s_Depth >= 128 )
	{
		Error::Error_1035_ExcessiveDepthComplexity( iter );
		return false;
	}

	// we expect a " quoted string
	if ( *iter != '"' )
	{
		Error::Error_1031_UnexpectedCharFollowingDirectiveName( iter, AStackString<>( "include" ), '"' ); 
		return false;
	}

	BFFIterator stringStart( iter );
	stringStart++; // first actual character

	// find end of string
	if ( iter.ParseToNext( '"' ) == false )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	// unescape and substitute variables
	AStackString<> include;
	if ( PerformVariableSubstitutions( stringStart, iter, include ) == false )
	{
		return false;
	}

	iter++; // skip closing quote before returning

	FLOG_INFO( "Including: %s\n", include.Get() );

	// open include

	// 1) Try current directory
	AStackString<> includeToUse;
	if (PathUtils::IsFullPath(include) == false)
	{
		const char * lastSlash = iter.GetFileName().FindLast( NATIVE_SLASH );
		lastSlash = lastSlash ? lastSlash : iter.GetFileName().FindLast( OTHER_SLASH );
		lastSlash = lastSlash ? ( lastSlash + 1 ): iter.GetFileName().Get(); // file only, truncate to empty
		includeToUse.Assign( iter.GetFileName().Get(), lastSlash );
	}
	includeToUse += include;
	AStackString<> includeToUseClean;
	NodeGraph::CleanPath( includeToUse, includeToUseClean );
	FileStream f;
	if ( f.Open( includeToUseClean.Get(), FileStream::READ_ONLY ) == false )
	{
		Error::Error_1032_UnableToOpenInclude( stringStart, includeToUseClean );
		return false;
	}

	// check if include uses "once" pragma
	if ( FBuild::Get().GetDependencyGraph().IsOneUseFile( includeToUseClean ) )
	{
		// already seen, and uses #once : don't include again
		return true;
	}

	uint64_t includeTimeStamp = FileIO::GetFileLastWriteTime( includeToUseClean );

	// read content of include
	const uint32_t fileSize = (uint32_t)f.GetFileSize();
	AutoPtr< char > mem( (char *)ALLOC( fileSize + 1 ) );
	if ( f.Read( mem.Get(), fileSize ) != fileSize )
	{
		Error::Error_1033_ErrorReadingInclude( stringStart, include, Env::GetLastErr() );
		return false;
	}
	mem.Get()[ fileSize ] = '\000'; // sentinel
	BFFParser parser;
	const bool pushStackFrame = false; // include is treated as if injected at this point
	return parser.Parse( mem.Get(), fileSize, includeToUseClean.Get(), includeTimeStamp, pushStackFrame ); 
}

// ParseDefineDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseDefineDirective( BFFIterator & iter )
{
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	// parse out token
	const BFFIterator tokenStart( iter );
	iter.SkipVariableName();
	if ( tokenStart.GetCurrent() == iter.GetCurrent() )
	{
		Error::Error_1007_ExpectedVariable( iter, nullptr );
		return false;
	}
	const BFFIterator tokenEnd( iter );

	AStackString<> token( tokenStart.GetCurrent(), tokenEnd.GetCurrent() );

	if ( BFFMacros::Get().Define( token ) == false )
	{
		Error::Error_1038_OverwritingTokenInDefine( tokenStart );
		return false;
	}

	FLOG_INFO( "Define macro <%s>", token.Get() );

	return true;
}

// ParseUndefDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseUndefDirective( BFFIterator & iter )
{
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	// parse out token
	const BFFIterator tokenStart( iter );
	iter.SkipVariableName();
	if ( tokenStart.GetCurrent() == iter.GetCurrent() )
	{
		Error::Error_1007_ExpectedVariable( iter, nullptr );
		return false;
	}
	const BFFIterator tokenEnd( iter );

	AStackString<> token( tokenStart.GetCurrent(), tokenEnd.GetCurrent() );

	if ( BFFMacros::Get().Undefine( token ) == false )
	{
		if ( token.BeginsWith( "__" ) )
		{
			Error::Error_1040_UndefOfBuiltInTokenNotAllowed( tokenStart );			
		}
		else
		{
			Error::Error_1039_UnknownTokenInUndef( tokenStart );
		}
		return false;
	}

	FLOG_INFO( "Undefine macro <%s>", token.Get() );

	return true;
}

// ParseIfDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseIfDirective( const BFFIterator & directiveStart, BFFIterator & iter )
{
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	bool negate = false;
	if ( *iter == '!' )
	{
		negate = true; // the condition will be inverted
		iter++; // skip '!'
		iter.SkipWhiteSpace(); // allow whitepace after '!'
		if ( iter.IsAtEnd() )
		{
			Error::Error_1012_UnexpectedEndOfFile( iter );
			return false;
		}
	}

	// parse out condition
	const BFFIterator conditionStart( iter );
	iter.SkipVariableName();
	if ( conditionStart.GetCurrent() == iter.GetCurrent() )
	{
		Error::Error_1007_ExpectedVariable( directiveStart, nullptr );
		return false;
	}
	const BFFIterator conditionEnd( iter );

	// Evaluate the condition
	bool result;
	if ( CheckIfCondition( conditionStart, conditionEnd, result ) == false )
	{
		return false; // CheckIfCondition will have emitted an error
	}

	// #ifndef ?
	if ( negate )
    {
		result = !( result );
    }

	if ( result )
	{
		++s_IfDepth; // Track that we're inside an if block
		return true; // continue parsing like normal
	}

	// Advance iterator past entire #if block
	size_t depth = 1; // handle nested ifs
	while ( depth > 0 )
	{
		// did we hit the end of the file?
		if ( iter.IsAtEnd() )
		{
			(void)directiveStart; // TODO: Show we're looking for matching endif to this
			Error::Error_1012_UnexpectedEndOfFile( iter ); // TODO:B better error for this?
			return false;
		}

		// find the next preprocessor directive
		iter.SkipWhiteSpace();
		if ( *iter == BFF_PREPROCESSOR_START )
		{
			iter++; // skip #
			iter.SkipWhiteSpace(); // allow whitespace between # and directive
			const BFFIterator directiveNameStart( iter );
			while ( iter.IsAtValidDirectiveNameCharacter() )
			{
				iter++;
			}
			const BFFIterator directiveNameEnd( iter );
			AStackString<> directiveName( directiveNameStart.GetCurrent(), directiveNameEnd.GetCurrent() );
			if ( directiveName == "endif" )
			{
				--depth;
			}
			else if ( directiveName == "if" )
			{
				++depth;
			}

			// continue to skip rest of line....
		}

		// skip rest of line
		while ( ( iter.IsAtEnd() == false ) &&
				( *iter != '\r' ) &&
				( *iter != '\n' ) )
		{
			iter++;
		}
	}

	return true;
}

// ParseEndIfDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseEndIfDirective( const BFFIterator & directiveStart )
{
	if ( s_IfDepth == 0 )
	{
		// unexpected endif
		Error::Error_1037_EndIfWithoutIf( directiveStart );
		return false;
	}

	s_IfDepth--;
	return true;
}

// CheckIfCondition
//------------------------------------------------------------------------------
bool BFFParser::CheckIfCondition( const BFFIterator & conditionStart, const BFFIterator & conditionEnd, bool & result )
{
	// trim condition
	AStackString<> condition( conditionStart.GetCurrent(), conditionEnd.GetCurrent() );

	result = BFFMacros::Get().IsDefined( condition );

	return true;
}

// ParseImportDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseImportDirective( const BFFIterator & directiveStart, BFFIterator & iter )
{
	iter.SkipWhiteSpace();

	// make sure we haven't hit the end of the file
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( directiveStart );
		return false;
	}

	// make sure this is a variable name
	if ( iter.IsAtValidVariableNameCharacter() == false )
	{
		Error::Error_1013_UnexpectedCharInVariableName( iter, nullptr );
		return false;
	}

	// find the end of the variable name
	const BFFIterator varNameStart( iter );
	iter.SkipVariableName();
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}
	const BFFIterator varNameEnd( iter );

	// sanity check it is a sensible length
	size_t varNameLen = varNameStart.GetDistTo( varNameEnd );
	if ( varNameLen > MAX_VARIABLE_NAME_LENGTH )
	{
		Error::Error_1014_VariableNameIsTooLong( iter, (uint32_t)varNameLen, (uint32_t)MAX_VARIABLE_NAME_LENGTH );
		return false;
	}
	AStackString<> varName( varNameStart.GetCurrent(), varNameEnd.GetCurrent() );

	// look for varName in system environment
	AStackString<> varValue;
	uint32_t varHash = 0;
	if ( FBuild::Get().ImportEnvironmentVar( varName.Get(), varValue, varHash ) == false )
	{
		Error::Error_1009_UnknownVariable( varNameStart, nullptr );
		return false;
	}

	// add the dot to variable name
	varName = ".";
	varName.Append( varNameStart.GetCurrent(), varNameLen );

	// import variable in current scope
	BFFStackFrame::SetVarString( varName, varValue, nullptr );
	FLOG_INFO( "Imported <string> variable '%s' with value '%s' from system environment", varName.Get(), varValue.Get() );

	return true;
}

// StoreVariableString
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableString( const AString & name,
									 const BFFIterator & valueStart, const BFFIterator & valueEnd,
									 const BFFIterator & operatorIter,
									 BFFStackFrame * frame )
{
	// unescape and subsitute embedded variables
	AStackString< 2048 > value;
	if ( PerformVariableSubstitutions( valueStart, valueEnd, value ) == false )
	{
		return false;
	}

	// are we concatenating?
	const BFFVariable * varToConcat = nullptr;
	if ( ( *operatorIter == BFF_VARIABLE_CONCATENATION ) || 
		 ( *operatorIter == BFF_VARIABLE_SUBTRACTION ) )
	{
		// find existing
		varToConcat = BFFStackFrame::GetVar( name, frame );
		if ( varToConcat == nullptr )
		{
			Error::Error_1026_VariableNotFoundForModification( operatorIter, name );
			return false;
		}

		// make sure types are compatible
		if ( varToConcat->IsString() )
		{
			// OK - can concat String to String
			AStackString< 1024 > finalValue( varToConcat->GetString() );
			if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
			{
				finalValue += value;
			}
			else
			{
				finalValue.Replace( value.Get(), "" );
			}

			BFFStackFrame::SetVarString( name, finalValue, frame );
			FLOG_INFO( "%s '%s' %s <String> variable '%s' with result '%s'", 
						( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "Appending" : "Removing",
						value.Get(), 
						( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "to" : "from",
						name.Get(), 
						finalValue.Get() );
			return true;
		}
		else if ( varToConcat->IsArrayOfStrings() )
		{
			// OK - can concat String to ArrayOfStrings
			Array< AString > finalValues( varToConcat->GetArrayOfStrings().GetSize() + 1, false );
			if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
			{
				finalValues = varToConcat->GetArrayOfStrings();
				finalValues.Append( value );
			}
			else
			{
				auto end = varToConcat->GetArrayOfStrings().End();
				for ( auto it=varToConcat->GetArrayOfStrings().Begin(); it!=end; ++it )
				{
					if ( *it != value ) // remove equal strings
					{
						finalValues.Append( *it );
					}
				}
			}

			BFFStackFrame::SetVarArrayOfStrings( name, finalValues, frame );
			FLOG_INFO( "%s '%s' %s <ArrayOfStrings> variable '%s' with result of %i items", 
						( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "Appending" : "Removing",
						value.Get(), 
						( *operatorIter == BFF_VARIABLE_CONCATENATION ) ? "to" : "from",
						name.Get(), 
						finalValues.GetSize() );
			return true;
		}
		else
		{
			Error::Error_1027_CannotModify( operatorIter, name, varToConcat->GetType(), BFFVariable::VAR_STRING );
			return false;
		}
	}

	// handle regular assignment of string
	BFFStackFrame::SetVarString( name, value, frame );
	FLOG_INFO( "Registered <string> variable '%s' with value '%s'", name.Get(), value.Get() );
	return true;
}

// StoreVariableArray
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableArray( const AString & name,
									const BFFIterator & valueStart, const BFFIterator & valueEnd,
									const BFFIterator & operatorIter,
									BFFStackFrame * frame )
{
	Array< AString > values( 32, true );
	Array< const BFFVariable * > structValues( 32, true );

	// are we concatenating?
	if ( ( *operatorIter == BFF_VARIABLE_CONCATENATION ) ||
		 ( *operatorIter == BFF_VARIABLE_SUBTRACTION ) )
	{
		// find existing
		const BFFVariable * var = BFFStackFrame::GetVar( name, frame );
		if ( var == nullptr )
		{
			Error::Error_1026_VariableNotFoundForModification( operatorIter, name );
			return false;
		}

		// make sure existing is an array
		if ( var->IsArrayOfStrings() )
		{
			// get values to start with
			values = var->GetArrayOfStrings();
		}
		else if ( var->IsArrayOfStructs() )
		{
			// get values to start with
			structValues = var->GetArrayOfStructs();
		}
		else
		{
			// TODO:B Improve this error to handle ArrayOfStructs case
			Error::Error_1027_CannotModify( operatorIter, name, var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
			return false;
		}
	}

	// Parse array of variables
	BFFIterator iter( valueStart );
	for (;;)
	{
		iter.SkipWhiteSpaceAndComments();

		// end?
		if ( iter.GetCurrent() == valueEnd.GetCurrent() )
		{
			break;
		}

		const char c = *iter;
		if ( ( c == '"' ) || ( c == '\'' ) )
		{
			// a quoted string

			// detect mistmatches
			if ( structValues.IsEmpty() == false )
			{
				// Mixed types in vector
				Error::Error_1034_OperationNotSupported( iter, 
														 BFFVariable::VAR_ARRAY_OF_STRUCTS,
														 BFFVariable::VAR_STRING,
														 operatorIter );
				return false;
			}

			// subtraction not supported on arrays
			if ( *operatorIter == BFF_VARIABLE_SUBTRACTION )
			{
				Error::Error_1034_OperationNotSupported( iter, BFFVariable::VAR_ARRAY_OF_STRINGS, BFFVariable::VAR_STRING, operatorIter );
				return false;
			}

			// a string
			BFFIterator elementValueStart( iter );
			iter.SkipString( c );
			ASSERT( iter.GetCurrent() <= valueEnd.GetCurrent() ); // should not be in this function if string is not terminated
			elementValueStart++; // move to start of actual content
			AStackString< 2048 > elementValue;

			// unescape and subsitute embedded variables
			if ( PerformVariableSubstitutions( elementValueStart, iter, elementValue ) == false )
			{
				return false;
			}

			values.Append( elementValue );

			iter++; // pass closing quote
		}
		else if ( c == BFF_DECLARE_VAR_INTERNAL ||
				  c == BFF_DECLARE_VAR_PARENT )
		{
			const BFFIterator elementStartValue = iter;

			// a variable
			AStackString< MAX_VARIABLE_NAME_LENGTH > varName;
			bool parentScope = false; // ignored, the behavior is the same
			if ( ParseVariableName( iter, varName, parentScope ) == false )
			{
				return false;
			}

			// Determine stack frame to use for Src var
			BFFStackFrame * srcFrame = BFFStackFrame::GetCurrent();			
			if ( c == BFF_DECLARE_VAR_PARENT )
			{
				srcFrame = BFFStackFrame::GetCurrent()->GetParent();
			}

			// get the variable
			const BFFVariable * var = srcFrame->GetVariableRecurse( varName );
			if ( var == nullptr )
			{
				Error::Error_1026_VariableNotFoundForModification( operatorIter, varName );
				return false;
			}

			// subtraction not supported on arrays
			if ( *operatorIter == BFF_VARIABLE_SUBTRACTION )
			{
				const BFFVariable::VarType dstType = structValues.IsEmpty() ? BFFVariable::VAR_ARRAY_OF_STRINGS : BFFVariable::VAR_ARRAY_OF_STRUCTS;
				const BFFVariable::VarType srcType = var->GetType();
				Error::Error_1034_OperationNotSupported( elementStartValue, dstType, srcType, operatorIter );
				return false;
    		}

			if ( var->IsString() || var->IsArrayOfStrings() )
			{
				// dest is consistent?
				if ( structValues.IsEmpty() == false )
				{
					// inconsistency
					Error::Error_1034_OperationNotSupported( elementStartValue, 
															 BFFVariable::VAR_ARRAY_OF_STRUCTS,
															 var->GetType(),
															 operatorIter );
					return false;
				}

				if ( var->IsString() )
				{
					values.Append( var->GetString() );
				}
				else
				{
					values.Append( var->GetArrayOfStrings() );
				}
			}
			else if ( var->IsStruct() || var->IsArrayOfStructs() )
			{
				// dest is consistent?
				if ( values.IsEmpty() == false )
				{
					// inconsistency
					Error::Error_1034_OperationNotSupported( elementStartValue, 
															 BFFVariable::VAR_ARRAY_OF_STRINGS,
															 var->GetType(),
															 operatorIter );
					return false;
				}

				if ( var->IsStruct() )
				{
					structValues.Append( var );
				}
				else
				{
					structValues.Append( var->GetArrayOfStructs() );
				}
			}
			else
			{
				Error::Error_1050_PropertyMustBeOfType( iter, nullptr, name.Get(), 
														var->GetType(),
														BFFVariable::VAR_STRING,
														BFFVariable::VAR_STRUCT );
				return false;
			}
		}
		else
		{
			Error::Error_1001_MissingStringStartToken( iter, nullptr );
			return false;
		}

		iter.SkipWhiteSpaceAndComments();
		if ( *iter == ',' ) // comma seperators are optional
		{ 
			iter++; 
		}

		// continue looking for more vars...
	}

	// should only have one populated array
	ASSERT( values.IsEmpty() || structValues.IsEmpty() );

	// Register this variable
	if ( structValues.IsEmpty() == false )
	{
		// structs
		BFFStackFrame::SetVarArrayOfStructs( name, structValues, frame );
		FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", name.Get(), structValues.GetSize() );
	}
	else
	{
		// strings (or unknown, so default to strings)
		BFFStackFrame::SetVarArrayOfStrings( name, values, frame );
		FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", name.Get(), values.GetSize() );
	}

	return true;
}

// StoreVariableStruct
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableStruct( const AString & name,
									 const BFFIterator & valueStart, const BFFIterator & valueEnd,
									 const BFFIterator & operatorIter,
									 BFFStackFrame * frame )
{
	// are we concatenating?
	if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
	{
		// concatenation of structs not supported
		Error::Error_1027_CannotModify( operatorIter, name, BFFVariable::VAR_STRUCT, BFFVariable::VAR_ANY );
		return false;
	}

	// create stack frame to capture variables
	BFFStackFrame stackFrame;

	// parse all the variables in the scope
	BFFParser subParser;
	BFFIterator subIter( valueStart );
	subIter.SetMax( valueEnd.GetCurrent() ); // limit to closing token
	if ( subParser.Parse( subIter ) == false )
	{
		return false; // error will be emitted by Parse
	}

	// get variables defined in the scope
	const Array< const BFFVariable * > & structMembers = stackFrame.GetLocalVariables();

	// Register this variable
	BFFStackFrame::SetVarStruct( name, structMembers, frame ? frame : stackFrame.GetParent() );
	FLOG_INFO( "Registered <struct> variable '%s' with %u members", name.Get(), structMembers.GetSize() );

	return true;
}


// StoreVariableBool
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableBool( const AString & name, bool value, BFFStackFrame * frame )
{
	// Register this variable
	BFFStackFrame::SetVarBool( name, value, frame );

	FLOG_INFO( "Registered <bool> variable '%s' with value '%s'", name.Get(), value ? "true" : "false" );

	return true;
}

// StoreVariableInt
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableInt( const AString & name, int value, BFFStackFrame * frame )
{
	BFFStackFrame::SetVarInt( name, value, frame );

	FLOG_INFO( "Registered <int> variable '%s' with value '%i'", name.Get(), value );

	return true;
}

// StoreVariableToVariable
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableToVariable( const AString & dstName, BFFIterator & iter, const BFFIterator & operatorIter, BFFStackFrame * dstFrame )
{
	AStackString< MAX_VARIABLE_NAME_LENGTH > srcName;

	bool srcParentScope = false;
	const BFFIterator varNameSrcStart( iter ); // Take note of start of var
	if ( ParseVariableName( iter, srcName, srcParentScope ) == false )
	{
		return false;
	}

	// find src var
	const BFFVariable * varSrc = nullptr;
	BFFStackFrame * const srcFrame = ( srcParentScope )
		? BFFStackFrame::GetParentDeclaration( srcName, nullptr, varSrc )
		: nullptr;

	if ( !srcParentScope )
	{
		varSrc = BFFStackFrame::GetVar( srcName, nullptr );
	}

	if ( ( srcParentScope && nullptr == srcFrame ) || ( nullptr == varSrc ) )
    {
    	Error::Error_1009_UnknownVariable( varNameSrcStart, nullptr );
    	return false;
    }

	// find dst var
	const BFFVariable * varDst = BFFStackFrame::GetVar( dstName, dstFrame );

	const bool concat = ( *operatorIter == BFF_VARIABLE_CONCATENATION );

	// concatenation?
	if ( concat )
	{
		// can only concatenate to existing vars
		if ( varDst == nullptr )
		{
			Error::Error_1026_VariableNotFoundForModification( operatorIter, dstName );
			return false;
		}
	}

	// if dst exists, types must match
	BFFVariable::VarType srcType = varSrc->GetType(); 
	BFFVariable::VarType dstType = BFFVariable::VAR_ANY; 
	if ( varDst )
	{
		dstType = varDst->GetType();
	}
	else 
	{
		ASSERT( concat == false );
		dstType = srcType;
	}

	// handle supported types

	if ( srcType != dstType )
	{
		// Mismatched - is there a supported conversion?

		// String to ArrayOfStrings
		if ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRINGS ) && 
			 ( srcType == BFFVariable::VAR_STRING ) )
		{
			uint32_t num = (uint32_t)( 1 + ( concat ? varDst->GetArrayOfStrings().GetSize() : 0 ) );
			Array< AString > values( num, false );
			if ( concat )
			{
				values.Append( varDst->GetArrayOfStrings() );
			}
			values.Append( varSrc->GetString() );

			BFFStackFrame::SetVarArrayOfStrings( dstName, values, dstFrame );
			FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), num );
			return true;
		}

		// Struct to ArrayOfStructs
		if ( ( dstType == BFFVariable::VAR_ARRAY_OF_STRUCTS ) && 
			 ( srcType == BFFVariable::VAR_STRUCT ) )
		{
			uint32_t num = (uint32_t)( 1 + ( concat ? varDst->GetArrayOfStructs().GetSize() : 0 ) );
			Array< const BFFVariable * > values( num, false );
			if ( concat )
			{
				values.Append( varDst->GetArrayOfStructs() );
			}
			values.Append( varSrc );

			BFFStackFrame::SetVarArrayOfStructs( dstName, values, dstFrame );
			FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), num );
			return true;
		}

	}
	else
	{
		// Matching Src and Dst

		if ( srcType == BFFVariable::VAR_STRING )
		{
			AStackString< 2048 > finalValue;
			if ( concat )
			{
				finalValue = varDst->GetString();
			}
			finalValue += varSrc->GetString();

			BFFStackFrame::SetVarString( dstName, finalValue, dstFrame );
			FLOG_INFO( "Registered <string> variable '%s' with value '%s'", dstName.Get(), finalValue.Get() );
			return true;
		}
	
		if ( srcType == BFFVariable::VAR_ARRAY_OF_STRINGS )
		{
			if ( concat )
			{
				const unsigned int num = (unsigned int)( varSrc->GetArrayOfStrings().GetSize() + varDst->GetArrayOfStrings().GetSize() );
				Array< AString > values( num, false );
				values.Append( varDst->GetArrayOfStrings() );
				values.Append( varSrc->GetArrayOfStrings() );
				BFFStackFrame::SetVarArrayOfStrings( dstName, values, dstFrame );
				FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), num );
			}
			else
			{
				BFFStackFrame::SetVarArrayOfStrings( dstName, varSrc->GetArrayOfStrings(), dstFrame );
				FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), (unsigned int)varSrc->GetArrayOfStrings().GetSize() );
			}
			return true;
		}

		if ( srcType == BFFVariable::VAR_ARRAY_OF_STRUCTS )
		{
			if ( concat )
			{
				const unsigned int num = (unsigned int)( varSrc->GetArrayOfStructs().GetSize() + varDst->GetArrayOfStructs().GetSize() );
				Array< const BFFVariable * > values( num, false );
				values.Append( varDst->GetArrayOfStructs() );
				values.Append( varSrc->GetArrayOfStructs() );
				BFFStackFrame::SetVarArrayOfStructs( dstName, values, dstFrame );
				FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), num );
			}
			else
			{
				BFFStackFrame::SetVarArrayOfStructs( dstName, varSrc->GetArrayOfStructs(), dstFrame );
				FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), (unsigned int)varSrc->GetArrayOfStructs().GetSize() );
			}
			return true;
		}

		if ( srcType == BFFVariable::VAR_INT )
		{
			int newVal( varSrc->GetInt() );
			if ( concat )
			{
				newVal += varDst->GetInt();
			}
			return StoreVariableInt( dstName, newVal, dstFrame );
		}

		if ( srcType == BFFVariable::VAR_BOOL )
		{
			// only assignment is supported
			if ( concat == false )
			{
				return StoreVariableBool( dstName, varSrc->GetBool(), dstFrame );
			}
		}

		if ( srcType == BFFVariable::VAR_STRUCT )
		{
			const Array< const BFFVariable * > & srcMembers = varSrc->GetStructMembers();
			if ( concat )
			{
				BFFVariable *const newVar = BFFStackFrame::ConcatVars( dstName, varSrc, varDst, dstFrame );
				FLOG_INFO( "Registered <struct> variable '%s' with %u members", dstName.Get(), newVar->GetStructMembers().GetSize() );
			}
			else
			{
				// Register this variable
				BFFStackFrame::SetVarStruct( dstName, srcMembers, dstFrame );
				FLOG_INFO( "Registered <struct> variable '%s' with %u members", dstName.Get(), srcMembers.GetSize() );
			}
			return true;
		}
	}

	Error::Error_1034_OperationNotSupported( varNameSrcStart, 
											 varDst ? varDst->GetType() : varSrc->GetType(),
											 varSrc->GetType(),
											 operatorIter );
	return false;
}

// PerformVariableSubstitutions
//------------------------------------------------------------------------------
/*static*/ bool BFFParser::PerformVariableSubstitutions( const BFFIterator & startIter,
											  const BFFIterator & endIter,
											  AString & value )
{
	AStackString< 4096 > output;

	BFFIterator src( startIter );
	BFFIterator end( endIter );

	while ( src < end )
	{
		switch ( *src )
		{
			case '^':
			{
				src++; // skip escape char
				if ( src < end )
				{
					output += *src; // append escaped character
				}
				break;
			}
			case '$':
			{
				BFFIterator firstToken( src );
				src++; // skip opening $

				// find matching $
				BFFIterator startName( src );
				const char * endName = nullptr;
				while ( src < end )
				{
					if ( *src == '$' )
					{
						endName = src.GetCurrent();
						break;
					}
					src++;
				}
				if ( ( endName == nullptr ) ||
					 ( ( endName - startName.GetCurrent() ) < 1 ) )
				{
					Error::Error_1028_MissingVariableSubstitutionEnd( firstToken );
					return false; 
				}
				AStackString< MAX_VARIABLE_NAME_LENGTH > varName( startName.GetCurrent(), endName );
				const BFFVariable * var = BFFStackFrame::GetVarAny( varName );
				if ( var == nullptr )
				{
					Error::Error_1009_UnknownVariable( startName, nullptr );
					return false; 
				}
				if ( var->IsString() == false )
				{
					Error::Error_1029_VariableForSubstitutionIsNotAString( startName, varName, var->GetType() );
					return false; 
				}
				output += var->GetString();
				break;
			}
			default:
			{
				output += *src;
				break;
			}
		}
		src++;
	}

	value = output;
	return true;
}

//------------------------------------------------------------------------------
