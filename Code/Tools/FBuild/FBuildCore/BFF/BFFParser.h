// BFFParser - loads BFF files, extracting build rules
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_BFFPARSER_H
#define FBUILD_BFFPARSER_H

// Includes
//------------------------------------------------------------------------------
#include "BFFIterator.h"

#include "Core/Env/Assert.h"
#include "Core/Strings/AString.h"
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class FileStream;

// BFFParser
//------------------------------------------------------------------------------
class BFFParser
{
public:
	explicit BFFParser();
	~BFFParser();

	// Parse BFF data
	// - data provided must be followed by a single null character
	// - size passed must exclude null character
	bool Parse( const char * dataWithSentinel, 
			    uint32_t sizeExcludingSentinel,
				const char * fileName,
				uint64_t fileTimeStamp,
				bool pushStackFrame = true );
	bool Parse( BFFIterator & iterator );

	enum { BFF_COMMENT_SEMICOLON = ';' };
	enum { BFF_COMMENT_SLASH = '/' };
	enum { BFF_DECLARE_VAR_INTERNAL = '.' };
	enum { BFF_VARIABLE_ASSIGNMENT = '=' };
	enum { BFF_VARIABLE_CONCATENATION = '+' };
	enum { BFF_START_ARRAY = '{' };
	enum { BFF_END_ARRAY = '}' };
	enum { BFF_FUNCTION_ARGS_OPEN = '(' };
	enum { BFF_FUNCTION_ARGS_CLOSE = ')' };
	enum { BFF_SCOPE_OPEN = '{' };
	enum { BFF_SCOPE_CLOSE = '}' };
	enum { BFF_STRUCT_OPEN = '[' };
	enum { BFF_STRUCT_CLOSE = ']' };
	enum { BFF_PREPROCESSOR_START = '#' };

	enum { MAX_VARIABLE_NAME_LENGTH = 64 };
	enum { MAX_FUNCTION_NAME_LENGTH = 64 };
	enum { MAX_DIRECTIVE_NAME_LENGTH = 64 };

	static bool PerformVariableSubstitutions( const BFFIterator & startIter, const BFFIterator & endIter, AString & value );

private:
	bool ParseUnnamedVariableConcatenation( BFFIterator & iter );
	bool ParseNamedVariableDeclaration( BFFIterator & parseIndex );
	bool ParseVariableDeclaration( BFFIterator & iter, const BFFIterator & varNameStart,
													   const BFFIterator & varNameEnd );
	bool ParseFunction( BFFIterator & parseIndex );
	bool ParseUnnamedScope( BFFIterator & iter );
	bool ParsePreprocessorDirective( BFFIterator & iter );
	bool ParseIncludeDirective( BFFIterator & iter );
	bool ParseIfDirective( const BFFIterator & directiveStart, BFFIterator & iter );
	bool ParseEndIfDirective( const BFFIterator & directiveStart );
	bool CheckIfCondition( const BFFIterator & conditionStart, const BFFIterator & conditionEnd, bool & result );

	bool StoreVariableString( const char * varNameStart, const char * varNameEnd, const BFFIterator & valueStart, const BFFIterator & valueEnd, const BFFIterator & operatorIter );
	bool StoreVariableArray( const char * varNameStart, const char * varNameEnd, const BFFIterator & valueStart, const BFFIterator & valueEnd, const BFFIterator & operatorIter );
	bool StoreVariableStruct( const char * varNameStart, const char * varNameEnd, const BFFIterator & valueStart, const BFFIterator & valueEnd, const BFFIterator & operatorIter );
	bool StoreVariableBool( const char * varNameStart, const char * varNameEnd, bool value );
	bool StoreVariableInt( const char * varNameStart, const char * varNameEnd, int value );
	bool StoreVariableToVariable( const char * varNameDstStart, const char * varNameDstEnd,
						  		  const BFFIterator & varNameSrcStart, const BFFIterator & varNameSrcEnd,
								  const BFFIterator & operatorIter );
	// store the last seen variable
	bool m_SeenAVariable;
	BFFIterator m_LastVarNameStart;
	BFFIterator m_LastVarNameEnd;

	// track recursion depth to detect recursion or excessive complexity
	static uint32_t s_Depth;

	// track nested preprocessor directives
	static uint32_t s_IfDepth;
};

//------------------------------------------------------------------------------
#endif // FBUILD_BFFPARSER_H
 