// BFFParser - loads BFF files, extracting build rules
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_BFFPARSER_H
#define FBUILD_BFFPARSER_H

// Includes
//------------------------------------------------------------------------------
#include "BFFIterator.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class FileStream;
class BFFStackFrame;
class NodeGraph;

// BFFParser
//------------------------------------------------------------------------------
class BFFParser
{
public:
	explicit BFFParser( NodeGraph & nodeGraph );
	~BFFParser();

	// Parse BFF data
	// - data provided must be followed by a single null character
	// - size passed must exclude null character
	bool Parse( const char * dataWithSentinel, 
			    uint32_t sizeExcludingSentinel,
				const char * fileName,
				uint64_t fileTimeStamp,
				uint64_t fileDataHash,
				bool pushStackFrame = true );
	bool Parse( BFFIterator & iterator );

	enum { BFF_COMMENT_SEMICOLON = ';' };
	enum { BFF_COMMENT_SLASH = '/' };
	enum { BFF_DECLARE_VAR_INTERNAL = '.' };
	enum { BFF_DECLARE_VAR_PARENT = '^' };
	enum { BFF_VARIABLE_ASSIGNMENT = '=' };
	enum { BFF_VARIABLE_CONCATENATION = '+' };
	enum { BFF_VARIABLE_SUBTRACTION = '-' };
	enum { BFF_START_ARRAY = '{' };
	enum { BFF_END_ARRAY = '}' };
	enum { BFF_FUNCTION_ARGS_OPEN = '(' };
	enum { BFF_FUNCTION_ARGS_CLOSE = ')' };
	enum { BFF_SCOPE_OPEN = '{' };
	enum { BFF_SCOPE_CLOSE = '}' };
	enum { BFF_STRUCT_OPEN = '[' };
	enum { BFF_STRUCT_CLOSE = ']' };
	enum { BFF_PREPROCESSOR_START = '#' };

	enum { MAX_VARIABLE_NAME_LENGTH = 256 };
	enum { MAX_FUNCTION_NAME_LENGTH = 64 };
	enum { MAX_DIRECTIVE_NAME_LENGTH = 64 };

	static bool PerformVariableSubstitutions( const BFFIterator & startIter, const BFFIterator & endIter, AString & value );
	static bool ParseVariableName( BFFIterator & iter, AString & name, bool & parentScope );

private:
	bool ParseUnnamedVariableModification( BFFIterator & iter );
	bool ParseNamedVariableDeclaration( BFFIterator & parseIndex );
	bool ParseVariableDeclaration( BFFIterator & iter, const AString & varName, BFFStackFrame * frame );
	bool ParseFunction( BFFIterator & parseIndex );
	bool ParseUnnamedScope( BFFIterator & iter );
	bool ParsePreprocessorDirective( BFFIterator & iter );
	bool ParseIncludeDirective( BFFIterator & iter );
	bool ParseDefineDirective( BFFIterator & iter );
	bool ParseUndefDirective( BFFIterator & iter );
	bool ParseIfDirective( const BFFIterator & directiveStart, BFFIterator & iter );
	bool ParseElseDirective( const BFFIterator & directiveStart, BFFIterator & iter );
	bool ParseToEndIf( BFFIterator & iter, bool allowElse );
	bool ParseEndIfDirective( const BFFIterator & directiveStart );
	bool CheckIfCondition( const BFFIterator & conditionStart, const BFFIterator & conditionEnd, bool & result );
	bool ParseImportDirective( const BFFIterator & directiveStart, BFFIterator & iter );

	bool StoreVariableString( const AString & name, const BFFIterator & valueStart, const BFFIterator & valueEnd, const BFFIterator & operatorIter, BFFStackFrame * frame );
	bool StoreVariableArray( const AString & name, const BFFIterator & valueStart, const BFFIterator & valueEnd, const BFFIterator & operatorIter, BFFStackFrame * frame );
	bool StoreVariableStruct( const AString & name, const BFFIterator & valueStart, const BFFIterator & valueEnd, const BFFIterator & operatorIter, BFFStackFrame * frame );
	bool StoreVariableBool( const AString & name, bool value, BFFStackFrame * frame );
	bool StoreVariableInt( const AString & name, int value, BFFStackFrame * frame );
	bool StoreVariableToVariable( const AString & dstName, BFFIterator & iter, const BFFIterator & operatorIter, BFFStackFrame * dstFrame );
	// store the last seen variable
	bool m_SeenAVariable;
	AStackString< MAX_VARIABLE_NAME_LENGTH > m_LastVarName;
	BFFStackFrame * m_LastVarFrame;
	NodeGraph & m_NodeGraph;

	// track recursion depth to detect recursion or excessive complexity
	static uint32_t s_Depth;

	// track nested preprocessor directives
	static uint32_t s_IfDepth;
private:
	BFFParser & operator = (const BFFParser &) = delete;
};

//------------------------------------------------------------------------------
#endif // FBUILD_BFFPARSER_H
