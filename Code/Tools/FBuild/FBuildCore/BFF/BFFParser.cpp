// BFFParser
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "BFFParser.h"
#include "BFFIterator.h"
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
			{
				if ( ParseNamedVariableDeclaration( iter ) == false )
				{
					return false;
				}
				continue;
			}
			case BFF_VARIABLE_CONCATENATION:
			{
				// concatenation to last used variable
				if ( ParseUnnamedVariableConcatenation( iter ) == false )
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

// ParseUnnamedVariableConcatenation
//------------------------------------------------------------------------------
bool BFFParser::ParseUnnamedVariableConcatenation( BFFIterator & iter )
{
	ASSERT( *iter == BFF_VARIABLE_CONCATENATION );

	// have we assigned a variable before?
	if ( m_SeenAVariable == false )
	{
		Error::Error_1011_UnnamedConcatMustFollowAssignment( iter );
		return false;
	}

	return ParseVariableDeclaration( iter, m_LastVarNameStart, m_LastVarNameEnd );
}

// ParseNamedVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseNamedVariableDeclaration( BFFIterator & iter )
{
	// skip over the declaration symbol
	ASSERT( *iter == BFF_DECLARE_VAR_INTERNAL );
	m_LastVarNameStart = iter; // include type token in var name
	iter++;

	// make sure we haven't hit the end of the file
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	// make sure immediately after the symbol starts a variable name
	if ( iter.IsAtValidVariableNameCharacter() == false )
	{
		Error::Error_1013_UnexpectedCharInVariableName( iter, nullptr );
		return false;
	}

	// find the end of the variable name
	iter.SkipVariableName();
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}
	m_LastVarNameEnd = iter;

	// sanity check it is a sensible length
	size_t varNameLen = m_LastVarNameStart.GetDistTo( m_LastVarNameEnd );
	if ( varNameLen > MAX_VARIABLE_NAME_LENGTH )
	{
		Error::Error_1014_VariableNameIsTooLong( iter, (uint32_t)varNameLen, (uint32_t)MAX_VARIABLE_NAME_LENGTH );
		return false;
	}

	// find the start of the assignment
	iter.SkipWhiteSpaceAndComments();
	if ( iter.IsAtEnd() )
	{
		Error::Error_1012_UnexpectedEndOfFile( iter );
		return false;
	}

	return ParseVariableDeclaration( iter, m_LastVarNameStart, m_LastVarNameEnd );
}

// ParseVariableDeclaration
//------------------------------------------------------------------------------
bool BFFParser::ParseVariableDeclaration( BFFIterator & iter, const BFFIterator & varNameStart,
															  const BFFIterator & varNameEnd )
{
	m_SeenAVariable = true;

	// look for an appropriate operator
	BFFIterator operatorIter( iter );
	bool concatenation = false;
	if ( *iter == BFF_VARIABLE_ASSIGNMENT )
	{
		// assignment
	}
	else if ( *iter == BFF_VARIABLE_CONCATENATION )
	{
		// concatenation
		concatenation = true;
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

	AStackString< 64 > varName( varNameStart.GetCurrent(), varNameEnd.GetCurrent() );

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
		if ( concatenation )
		{
			Error::Error_1027_CannotConcatenate( operatorIter, varName, BFFVariable::VAR_ANY, BFFVariable::VAR_INT );
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
		return StoreVariableInt( varNameStart.GetCurrent(), varNameEnd.GetCurrent(), i );
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
					if ( concatenation )
					{
						Error::Error_1027_CannotConcatenate( operatorIter, varName, BFFVariable::VAR_ANY, BFFVariable::VAR_BOOL );
						return false;
					}
					return StoreVariableBool( varNameStart.GetCurrent(), varNameEnd.GetCurrent(), true );
				}
				else if ( value == "false" )
				{
					if ( concatenation )
					{
						Error::Error_1027_CannotConcatenate( operatorIter, varName, BFFVariable::VAR_ANY, BFFVariable::VAR_BOOL );
						return false;
					}
					return StoreVariableBool( m_LastVarNameStart.GetCurrent(), m_LastVarNameEnd.GetCurrent(), false );
				}
			}
		}

		// not a valid bool value
	}
	else if ( *iter == '.' )
	{
		// assignment of one variable to another
		BFFIterator varNameStartSrc( iter );
		iter++; // skip '.'
		if ( iter.IsAtValidVariableNameCharacter() == false )
		{
			Error::Error_1013_UnexpectedCharInVariableName( iter, nullptr );
			return false;
		}
		iter.SkipVariableName();
		return StoreVariableToVariable( varNameStart.GetCurrent(), varNameEnd.GetCurrent(), 
										varNameStartSrc, iter, operatorIter );
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
			result = StoreVariableArray( varNameStart.GetCurrent(), varNameEnd.GetCurrent(), 
										 openTokenPos, iter, operatorIter );
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
			result = StoreVariableStruct( varNameStart.GetCurrent(), varNameEnd.GetCurrent(), 
										  openTokenPos, iter, operatorIter );
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
			result = StoreVariableString( varNameStart.GetCurrent(), varNameEnd.GetCurrent(), 
										  openTokenPos, iter, operatorIter );
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
	else if ( directive == "if" )
	{
		return ParseIfDirective( directiveStart, iter );
	}
	else if ( directive == "endif" )
	{
		return ParseEndIfDirective( directiveStartIter );
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

	AStackString<> include( stringStart.GetCurrent(), iter.GetCurrent() );
	iter++; // skip closing quote before returning

	FLOG_INFO( "Including: %s\n", include.Get() );

	// open include

	// 1) Try current directory
	const char * lastSlash = iter.GetFileName().FindLast( NATIVE_SLASH );
	lastSlash = lastSlash ? lastSlash : iter.GetFileName().FindLast( OTHER_SLASH );
	lastSlash = lastSlash ? ( lastSlash + 1 ): iter.GetFileName().Get(); // file only, truncate to empty
	AStackString<> includeToUse( iter.GetFileName().Get(), lastSlash );
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

// ParseIfDirective
//------------------------------------------------------------------------------
bool BFFParser::ParseIfDirective( const BFFIterator & directiveStart, BFFIterator & iter )
{
	// parse out condition
	const BFFIterator conditionStart( iter );
	while ( ( iter.IsAtEnd() == false ) &&
			( *iter != '\r' ) &&
			( *iter != '\n' ) )
	{
		iter++;
	}
	const BFFIterator conditionEnd( iter );

	// Evaluate the condition
	bool result;
	if ( CheckIfCondition( conditionStart, conditionEnd, result ) == false )
	{
		return false; // CheckIfCondition will have emitted an error
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
	condition.Replace( '\t', ' ' );
	condition.Replace( " ", "" );

	result = false;

	// For now we only support trivial pre-defined expressions - TODO:B Support more complex expressions
	if ( condition == "__WINDOWS__" )
	{
		#if defined( __WINDOWS__ )
			result = true;
		#endif
		return true;
	}
	if ( condition == "__LINUX__" )
	{
		#if defined( __LINUX__ )
			result = true;
		#endif
		return true;
	}
	if ( condition == "__OSX__" )
	{
		#if defined( __OSX__ )
			result = true;
		#endif
		return true;
	}

	// We found an expression we don't understand
	Error::Error_1036_UnknownTokenInIfDirective( conditionStart );
	return false;
}

// StoreVariableString
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableString( const char * varNameStart, const char * varNameEnd, 
									 const BFFIterator & valueStart, const BFFIterator & valueEnd,
									 const BFFIterator & operatorIter )
{
	// unescape and subsitute embedded variables
	AStackString< 2048 > value;
	if ( PerformVariableSubstitutions( valueStart, valueEnd, value ) == false )
	{
		return false;
	}

	ASSERT( varNameStart );
	ASSERT( varNameEnd );
	AStackString< MAX_VARIABLE_NAME_LENGTH > name( varNameStart, varNameEnd );

	// are we concatenating?
	const BFFVariable * varToConcat = nullptr;
	if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
	{
		// find existing
		varToConcat = BFFStackFrame::GetVar( name );
		if ( varToConcat == nullptr )
		{
			Error::Error_1026_VariableNotFoundForConcatenation( operatorIter, name );
			return false;
		}

		// make sure types are compatible
		if ( varToConcat->IsString() )
		{
			// OK - can concat String to String
			AStackString< 1024 > finalValue( varToConcat->GetString() );
			finalValue += value;

			BFFStackFrame::SetVarString( name, finalValue );
			FLOG_INFO( "Appended '%s' to <String> variable '%s' with result '%s'", value.Get(), name.Get(), finalValue.Get() );
			return true;
		}
		else if ( varToConcat->IsArrayOfStrings() )
		{
			// OK - can concat String to ArrayOfStrings
			Array< AString > finalValues( varToConcat->GetArrayOfStrings().GetSize() + 1, false );
			finalValues = varToConcat->GetArrayOfStrings();
			finalValues.Append( value );

			BFFStackFrame::SetVarArrayOfStrings( name, finalValues );
			FLOG_INFO( "Appended '%s' to <ArrayOfStrings> variable '%s' with result of %i items", value.Get(), name.Get(), finalValues.GetSize() );
			return true;
		}
		else
		{
			Error::Error_1027_CannotConcatenate( operatorIter, name, varToConcat->GetType(), BFFVariable::VAR_STRING );
			return false;
		}
	}

	// handle regular assignment of string
	BFFStackFrame::SetVarString( name, value );
	FLOG_INFO( "Registered <string> variable '%s' with value '%s'", name.Get(), value.Get() );
	return true;
}

// StoreVariableArray
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableArray( const char * varNameStart, const char * varNameEnd, 
									const BFFIterator & valueStart, const BFFIterator & valueEnd,
									const BFFIterator & operatorIter )
{
	ASSERT( varNameStart );
	ASSERT( varNameEnd );
	AStackString< MAX_VARIABLE_NAME_LENGTH > name( varNameStart, varNameEnd );

	Array< AString > values( 32, true );
	Array< const BFFVariable * > structValues( 32, true );

	// are we concatenating?
	if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
	{
		// find existing
		const BFFVariable * var = BFFStackFrame::GetVar( name );
		if ( var == nullptr )
		{
			Error::Error_1026_VariableNotFoundForConcatenation( operatorIter, name );
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
			Error::Error_1027_CannotConcatenate( operatorIter, name, var->GetType(), BFFVariable::VAR_ARRAY_OF_STRINGS );
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
		else if ( c == '.' )
		{
			// a variable
			BFFIterator elementStartValue( iter );
			iter++; // move to start of actual variable name
			iter.SkipVariableName();

			// get the variable
			AStackString<> varName( elementStartValue.GetCurrent(), iter.GetCurrent() );
			const BFFVariable * var = BFFStackFrame::GetVar( varName );
			if ( var == nullptr )
			{
				Error::Error_1026_VariableNotFoundForConcatenation( operatorIter, varName );
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
		BFFStackFrame::SetVarArrayOfStructs( name, structValues );
		FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", name.Get(), structValues.GetSize() );
	}
	else
	{
		// strings (or unknown, so default to strings)
		BFFStackFrame::SetVarArrayOfStrings( name, values );
		FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", name.Get(), values.GetSize() );
	}

	return true;
}

// StoreVariableStruct
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableStruct( const char * varNameStart, const char * varNameEnd, 
									 const BFFIterator & valueStart, const BFFIterator & valueEnd,
									 const BFFIterator & operatorIter )
{
	ASSERT( varNameStart );
	ASSERT( varNameEnd );
	AStackString< MAX_VARIABLE_NAME_LENGTH > name( varNameStart, varNameEnd );

	// are we concatenating?
	if ( *operatorIter == BFF_VARIABLE_CONCATENATION )
	{
		// concatenation of structs not supported
		Error::Error_1027_CannotConcatenate( operatorIter, name, BFFVariable::VAR_STRUCT, BFFVariable::VAR_ANY );
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
	BFFStackFrame::SetVarStruct( name, structMembers, stackFrame.GetParent() );
	FLOG_INFO( "Registered <struct> variable '%s' with %u members", name.Get(), structMembers.GetSize() );

	return true;
}


// StoreVariableBool
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableBool( const char * varNameStart, const char * varNameEnd, bool value )
{
	// Register this variable
	AStackString< MAX_VARIABLE_NAME_LENGTH > name( varNameStart, varNameEnd );
	BFFStackFrame::SetVarBool( name, value );

	FLOG_INFO( "Registered <bool> variable '%s' with value '%s'", name.Get(), value ? "true" : "false" );

	return true;
}

// StoreVariableInt
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableInt( const char * varNameStart, const char * varNameEnd, int value )
{
	AStackString< MAX_VARIABLE_NAME_LENGTH > name( varNameStart, varNameEnd );
	BFFStackFrame::SetVarInt( name, value );

	FLOG_INFO( "Registered <int> variable '%s' with value '%i'", name.Get(), value );

	return true;
}

// 
//------------------------------------------------------------------------------
bool BFFParser::StoreVariableToVariable( const char * varNameDstStart, const char * varNameDstEnd,
						  				 const BFFIterator & varNameSrcStart, const BFFIterator & varNameSrcEnd,
										 const BFFIterator & operatorIter )
{
	// find vars
	AStackString<> dstName( varNameDstStart, varNameDstEnd );
	AStackString<> srcName( varNameSrcStart.GetCurrent(), varNameSrcEnd.GetCurrent() );
	const BFFVariable * varDst = BFFStackFrame::GetVar( dstName );
	const BFFVariable * varSrc = BFFStackFrame::GetVar( srcName );

	// src var unknown?
	if ( varSrc == nullptr )
	{
		Error::Error_1009_UnknownVariable( varNameSrcStart, nullptr );
		return false;
	}

	const bool concat = ( *operatorIter == BFF_VARIABLE_CONCATENATION );

	// concatenation?
	if ( concat )
	{
		// can only concatenate to existing vars
		if ( varDst == nullptr )
		{
			Error::Error_1026_VariableNotFoundForConcatenation( operatorIter, dstName );
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

			BFFStackFrame::SetVarArrayOfStrings( dstName, values );
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

			BFFStackFrame::SetVarArrayOfStructs( dstName, values );
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

			BFFStackFrame::SetVarString( dstName, finalValue );
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
				BFFStackFrame::SetVarArrayOfStrings( dstName, values );
				FLOG_INFO( "Registered <ArrayOfStrings> variable '%s' with %u elements", dstName.Get(), num );
			}
			else
			{
				BFFStackFrame::SetVarArrayOfStrings( dstName, varSrc->GetArrayOfStrings() );
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
				BFFStackFrame::SetVarArrayOfStructs( dstName, values );
				FLOG_INFO( "Registered <ArrayOfStructs> variable '%s' with %u elements", dstName.Get(), num );
			}
			else
			{
				BFFStackFrame::SetVarArrayOfStructs( dstName, varSrc->GetArrayOfStructs() );
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
			return StoreVariableInt( varNameDstStart, varNameDstEnd, newVal );
		}

		if ( srcType == BFFVariable::VAR_BOOL )
		{
			// only assignment is supported
			if ( concat == false )
			{
				return StoreVariableBool( varNameDstStart, varNameDstEnd, varSrc->GetBool() );
			}
		}

		if ( srcType == BFFVariable::VAR_STRUCT )
		{
			const Array< const BFFVariable * > & srcMembers = varSrc->GetStructMembers();
			if ( concat )
			{
				// set all the variable in 
				ASSERT( varDst ); // have checked this earlier
				const Array< const BFFVariable * > & dstMembers = varDst->GetStructMembers();

				// we'll take all the src members
				Array< const BFFVariable * > allMembers( srcMembers.GetSize() + dstMembers.GetSize(), false );
				allMembers = srcMembers;

				// and keep original (dst) members where the name doesn't clash
				for ( const BFFVariable ** it = dstMembers.Begin(); it != dstMembers.End(); ++it )
				{
					// if the existing members with conflicting names
					bool exists = false;
					for ( const BFFVariable ** it2 = srcMembers.Begin(); it2 != srcMembers.End(); ++it2 )
					{
						if ( ( *it2 )->GetName() == ( *it )->GetName() )
						{
							// already exists in src data, which will override existing in dst
							exists = true;
							break;
						}
					}
					if ( exists == false )
					{
						// keep existing dst since there is no src override
						allMembers.Append( *it );
					}
				}

				BFFStackFrame::SetVarStruct( dstName, allMembers );
				FLOG_INFO( "Registered <struct> variable '%s' with %u members", dstName.Get(), allMembers.GetSize() );
			}
			else
			{
				// Register this variable
				BFFStackFrame::SetVarStruct( dstName, srcMembers );
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
