// Error
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Error.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFIterator.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#include <stdarg.h>
#include <stdio.h>

// Error_1001_MissingStringStartToken
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1001_MissingStringStartToken( const BFFIterator & iter,
														const Function * function )
{
	FormatError( iter, 1001u, function, "Missing string start token \" or '." );
}

// Error_1002_MatchingClosingTokenNotFound
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1002_MatchingClosingTokenNotFound( const BFFIterator & iter,
																const Function * function,
																char expectedChar )
{
	FormatError( iter, 1002u, function, "Matching closing token %c not found.",
									 expectedChar );
}

// Error_1003_EmptyStringNotAllowedInHeader
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1003_EmptyStringNotAllowedInHeader( const BFFIterator & iter,
													          const Function * function )
{
	FormatError( iter, 1003u, function, "Empty string not allowed in Function header." );
}

// Error_1004_EmptyStringPropertyNotAllowed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1004_EmptyStringPropertyNotAllowed( const BFFIterator & iter,
															  const Function * function,
															  const char * propertyName )
{
	FormatError( iter, 1004u, function, "Empty string not allowed for property '%s'.",
									 propertyName );
}

// Error_1005_UnsupportedNodeType
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1005_UnsupportedNodeType( const BFFIterator & iter,
													const Function * function,
													const char * propertyName,
													const AString & nodeName,
													Node::Type nodeType )
{
	FormatError( iter, 1005u, function, "Unsupported node type in '%s'. (Node: '%s', Type: '%s')",
									 propertyName,
									 nodeName.Get(),
									 Node::GetTypeName( nodeType ) );
}

// Error_1006_NothingToBuild
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1006_NothingToBuild( const BFFIterator & iter,
											   const Function * function )
{
	FormatError( iter, 1006u, function, "Nothing defined to be built." );
}

// Error_1007_ExpectedVariable
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1007_ExpectedVariable( const BFFIterator & iter,
													const Function * function )
{
	FormatError( iter, 1007u, function, "Expected a variable at this location." );
}

// Error_1008_VariableOfWrongType
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1008_VariableOfWrongType( const BFFIterator & iter,
													   const Function * function,
													   BFFVariable::VarType expectedType,
													   BFFVariable::VarType foundType )
{
	FormatError( iter, 1008u, function, "Expected a variable of type '%s', but got '%s'.",
										BFFVariable::GetTypeName( expectedType ), 
										BFFVariable::GetTypeName( foundType ) );
}

// Error_1009_UnknownVariable
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1009_UnknownVariable( const BFFIterator & iter,
												const Function * function )
{
	FormatError( iter, 1009u, function, "Unknown variable." );
}

// Error_1010_UnknownConstruct
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1010_UnknownConstruct( const BFFIterator & iter )
{
	FormatError( iter, 1010u, nullptr, "Unknown construct." );
}

// Error_1011_UnnamedModifcationMustFollowAssignment
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1011_UnnamedModifcationMustFollowAssignment( const BFFIterator & iter )
{
	FormatError( iter, 1011u, nullptr, "Unnamed modification must follow a variable assignment in the same scope." );
}

// Error_1012_UnexpectedEndOfFile
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1012_UnexpectedEndOfFile( const BFFIterator & iter )
{
	FormatError( iter, 1012u, nullptr, "Unexpected end of file." );
}

// Error_1013_UnexpectedCharInVariableName
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1013_UnexpectedCharInVariableName( const BFFIterator & iter,
															  const Function * function )
{
	FormatError( iter, 1013u, function, "Unexpected character '%c' in variable name.", *iter );
}

// Error_1014_VariableNameIsTooLong
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1014_VariableNameIsTooLong( const BFFIterator & iter, uint32_t length, uint32_t maxLength )
{
	FormatError( iter, 1014u, nullptr, "Variable name is too long. (Length: %u, MaxLength: %u)", length, maxLength );
}

// Error_1015_UnknownFunction
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1015_UnknownFunction( const BFFIterator & iter )
{
	FormatError( iter, 1015u, nullptr, "Unknown Function." );
}

// Error_1016_UnexepectedCharFollowingVariableName
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1016_UnexepectedCharFollowingVariableName( const BFFIterator & iter )
{
	FormatError( iter, 1016u, nullptr, "Unexpected character '%c' following variable name. (Expected operator)", *iter );
}

// Error_1017_UnexepectedCharInVariableValue
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1017_UnexepectedCharInVariableValue( const BFFIterator & iter )
{
	FormatError( iter, 1017u, nullptr, "Unexpected character '%c' in variable value.", *iter );
}

// Error_1018_IntegerDeclarationCouldNotBeParsed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1018_IntegerValueCouldNotBeParsed( const BFFIterator & iter )
{
	FormatError( iter, 1018u, nullptr, "Integer value could not be parsed." );
}

// Error_1020_FunctionCanOnlyBeInvokedOnce
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1020_FunctionCanOnlyBeInvokedOnce( const BFFIterator & iter, const Function * function )
{
	FormatError( iter, 1020u, function, "Function can only be invoked once." );
}

// Error_1021_UnexpectedHeaderForFunction
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1021_UnexpectedHeaderForFunction( const BFFIterator & iter, const Function * function )
{
	FormatError( iter, 1021u, function, "Unexpected Function header." );
}

// Error_1022_MissingFunctionHeaderCloseToken
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1022_MissingFunctionHeaderCloseToken( const BFFIterator & iter, const Function * function )
{
	FormatError( iter, 1022u, function, "Missing Function header close token '%c'.", BFFParser::BFF_FUNCTION_ARGS_CLOSE );
}

// Error_1023_FunctionRequiresAHeader
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1023_FunctionRequiresAHeader( const BFFIterator & iter,
														 const Function * function )
{
	FormatError( iter, 1023u, function, "Function requires a header." );
}

// Error_1024_FunctionRequiresABody
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1024_FunctionRequiresABody( const BFFIterator & iter,
													   const Function * function )
{
	FormatError( iter, 1024u, function, "Function requires a body." );
}

// Error_1025_MissingScopeCloseToken
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1025_MissingScopeCloseToken( const BFFIterator & iter,
														  const Function * function )
{
	FormatError( iter, 1025u, function, "Missing scope close token '%c'.", BFFParser::BFF_SCOPE_CLOSE );
}

// Error_1026_VariableNotFoundForModification
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1026_VariableNotFoundForModification( const BFFIterator & iter,
																  const AString & variableName )
{
	FormatError( iter, 1026u, nullptr, "Variable '%s' not found for modification.", variableName.Get() );
}

// Error_1027_CannotModify
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1027_CannotModify( const BFFIterator & iter,
												   const AString & variableName,
												   BFFVariable::VarType dstType,
												   BFFVariable::VarType srcType )
{
	FormatError( iter, 1027u, nullptr, "Cannot use <%s> to modify <%s> for Variable '%s'.",
									 BFFVariable::GetTypeName( srcType ), 
									 BFFVariable::GetTypeName( dstType ),
									 variableName.Get() );
}

// Error_1028_MissingVariableSubstitutionEnd
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1028_MissingVariableSubstitutionEnd( const BFFIterator & iter )
{
	FormatError( iter, 1028u, nullptr, "Cannot find '$' to end variable substitution." );
}

// Error_1029_VariableForSubstitutionIsNotAString
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1029_VariableForSubstitutionIsNotAString( const BFFIterator & iter,
																	 const AString & variableName,
																	 BFFVariable::VarType varType )
{
	FormatError( iter, 1029u, nullptr, "Variable for substitution '%s' is not a string (Type: <%s>).",
									 variableName.Get(),
									 BFFVariable::GetTypeName( varType ) );
}

// Error_1030_UnknownDirective
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1030_UnknownDirective( const BFFIterator & iter,
													const AString & directiveName )
{
	FormatError( iter, 1030u, nullptr, "Unknown directive '%s'.",
									 directiveName.Get() );
}

// Error_1031_UnexpectedCharFollowingDirectiveName
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1031_UnexpectedCharFollowingDirectiveName( const BFFIterator & iter,
																		const AString & directive,
																		char expectedChar )
{
	FormatError( iter, 1031u, nullptr, "Unknown char '%c' following '%s' directive. (Expected '%c').",
									 *iter, 
									 directive.Get(),
									 expectedChar );
}

// Error_1032_UnableToOpenInclude
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1032_UnableToOpenInclude( const BFFIterator & iter,
													   const AString & include )
{
	FormatError( iter, 1032u, nullptr, "Unable to open include '%s'.",
									 include.Get() );
}

// Error_1033_ErrorReadingInclude
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1033_ErrorReadingInclude( const BFFIterator & iter,
													   const AString & include,
													   uint32_t errorCode )
{
	FormatError( iter, 1033u, nullptr, "Error reading include '%s' (Error: %u).",
									   include.Get(),
									   errorCode );
}

// Error_1034_OperationNotSupported
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1034_OperationNotSupported( const BFFIterator & rhsIter,
														 BFFVariable::VarType lhs,
														 BFFVariable::VarType rhs,
														 const  BFFIterator & operatorIter )
{
	const char * operation  = "???"; // should never be used
	switch ( *operatorIter )
	{
		case BFFParser::BFF_VARIABLE_ASSIGNMENT:	operation = "="; break;
		case BFFParser::BFF_VARIABLE_CONCATENATION: operation = "+"; break;
		case BFFParser::BFF_VARIABLE_SUBTRACTION:	operation = "-"; break;
		default:									ASSERT( false ); break;
	}

	FormatError( rhsIter, 1034u, nullptr, "Operation not supported: '%s' %s '%s'.",
									   BFFVariable::GetTypeName( lhs ),
									   operation,
									   BFFVariable::GetTypeName( rhs ) );
}

// Error_1035_ExcessiveDepthComplexity
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1035_ExcessiveDepthComplexity( const BFFIterator & iter )
{
	FormatError( iter, 1035u, nullptr, "Excessive depth complexity when parsing BFFs. Check for resursive includes or simplify config." );
}

// Error_1036_UnknownTokenInIfDirective
//------------------------------------------------------------------------------
///*static*/ void Error::Error_1036_UnknownTokenInIfDirective( const BFFIterator & iter )
//{
//	FormatError( iter, 1036u, nullptr, "Unknown token in #if directive." );
//}

// Error_1037_EndIfWithoutIf
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1037_EndIfWithoutIf( const BFFIterator & iter )
{
	FormatError( iter, 1037u, nullptr, "#endif without matching #if." );
}

// Error_1038_OverwritingTokenInDefine
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1038_OverwritingTokenInDefine( const BFFIterator & iter )
{
	FormatError( iter, 1038u, nullptr, "Overwriting already defined token in #define directive." );
}

// Error_1039_UnknownTokenInUndef
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1039_UnknownTokenInUndef( const BFFIterator & iter )
{
	FormatError( iter, 1039u, nullptr, "Unknown token in #undef directive." );
}

// Error_1040_UndefOfBuiltInTokenNotAllowed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1040_UndefOfBuiltInTokenNotAllowed( const BFFIterator & iter )
{
	FormatError( iter, 1040u, nullptr, "#undef of built-in token not allowed." );
}

// Error_1041_ElseWithoutIf
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1041_ElseWithoutIf( const BFFIterator & iter )
{
	FormatError( iter, 1041u, nullptr, "#else without matching #if." );
}

// Error_1050_PropertyMustBeString
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1050_PropertyMustBeOfType( const BFFIterator & iter,
													  const Function * function,
													  const char * propertyName,
													  BFFVariable::VarType foundType,
													  BFFVariable::VarType requiredType,
													  BFFVariable::VarType alternateRequiredType )
{
	if ( alternateRequiredType == BFFVariable::VAR_ANY )
	{
		FormatError( iter, 1050u, function, "Property '%s' must be of type <%s> (found <%s>).",
										  propertyName,
										  BFFVariable::GetTypeName( requiredType ),
										  BFFVariable::GetTypeName( foundType ) );
	}
	else
	{
		FormatError( iter, 1050u, function, "Property '%s' must be of type <%s> or <%s> (found <%s>).",
										  propertyName,
										  BFFVariable::GetTypeName( requiredType ),
										  BFFVariable::GetTypeName( alternateRequiredType ),
										  BFFVariable::GetTypeName( foundType ) );
	}
}

// Error_1054_IntegerOutOfRange
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1054_IntegerOutOfRange( const BFFIterator & iter,
												   const Function * function,
												   const char * propertyName,
												   int rangeMin,
												   int rangeMax )
{
	FormatError( iter, 1054u, function, "Integer '%s' must be in range %i to %i.",
									  propertyName,
									  rangeMin, rangeMax );
}

// Error_1060_CantModifyFrozenVar
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1060_CantModifyFrozenVar( const BFFIterator & iter, const Function * function, const BFFVariable * var )
{
	FormatError( iter, 1060u, function, "Can't modify frozen variable '%s'", var->GetName().Get() );
}

// Error_1100_AlreadyDefined
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1100_AlreadyDefined( const BFFIterator & iter,
												 const Function * function,
												 const AString & name )
{
	FormatError( iter, 1100u, function, "Target '%s' already defined.",
									   name.Get() );
}

// Error_1101_MissingProperty
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1101_MissingProperty( const BFFIterator & iter,
												  const Function * function,
												  const AString & name )
{
	FormatError( iter, 1101u, function, "Missing required property '%s'.",
									   name.Get() );
}

// Error_1102_UnexpectedType
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1102_UnexpectedType( const BFFIterator & iter,
												 const Function * function,
												 const char * propertyName,
												 const AString & name,
												 Node::Type foundType,
												 Node::Type expectedType )
{
	FormatError( iter, 1102u, function, "'%s' ('%s') is of unexpected type '%s'. Expected '%s'.",
									   propertyName,
									   name.Get(),
									   Node::GetTypeName( foundType ),
									   Node::GetTypeName( expectedType ) );
}

// Error_1103_NotAFile
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1103_NotAFile( const BFFIterator & iter,
										   const Function * function,
										   const char * propertyName,
										   const AString & name,
										   Node::Type foundType )
{
	FormatError( iter, 1103u, function, "'%s' ('%s') is a '%s', which does not describe a file.",
									   propertyName,
									   name.Get(),
									   Node::GetTypeName( foundType ) );
}

// Error_1104_MissingTarget
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1104_TargetNotDefined( const BFFIterator & iter,
												   const Function * function,
											       const char * propertyName,
												   const AString & name )
{
	FormatError( iter, 1104u, function, "'%s' ('%s') is not defined.",
									   propertyName,
									   name.Get() );
}

// Error_1105_PathNotAllowed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1105_PathNotAllowed( const BFFIterator & iter,
												  const Function * function,
												  const char * propertyName,
												  const AString & propertyValue )

{
	FormatError( iter, 1105u, function, "Path not allowed for '%s' ('%s').",
									   propertyName,
									   propertyValue.Get() );
}

// Error_1106_MissingRequiredToken
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1106_MissingRequiredToken( const BFFIterator & iter,
														const Function * function,
														const char * propertyName,
														const char * token )
{
	FormatError( iter, 1106u, function, "Property '%s' is missing required token '%s'.",
										propertyName,
									    token );
}

// Error_1200_ExpectedVar // TODO:C Remove (Deprecated by 1007)
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1200_ExpectedVar( const BFFIterator & iter, const Function * function )
{
	FormatError( iter, 1200u, function, "Expected a variable at this location." );
}

// Error_1201_MissingIn
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1201_MissingIn( const BFFIterator & iter, const Function * function )
{
	FormatError( iter, 1201u, function, "Missing 'in' at this location." );
}

// Error_1202_ExpectedVarFollowingIn
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1202_ExpectedVarFollowingIn( const BFFIterator & iter, const Function * function )
{
	FormatError( iter, 1202u, function, "Expected a variable following 'in'." );
}

// Error_1204_LoopVariableLengthsDiffer
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1204_LoopVariableLengthsDiffer( const BFFIterator & iter, 
															const Function * function, 
															const char * variableName,
															uint32_t foundSize,
															uint32_t expectedSize )
{
	FormatError( iter, 1204u, function, "Variable '%s' contains %u elements, but loop is for %u elements.",
									   variableName,
									   foundSize,
									   expectedSize );
}

// Error_1300_MissingPCHArgs
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1300_MissingPCHArgs( const BFFIterator & iter, 
												 const Function * function )
{
	FormatError( iter, 1300u, function, "Precompiled Header use requires 'PCHInputFile', 'PCHOutputFile' and 'PCHOptions' parameters." );
}

// Error_1301_AlreadyDefinedPCH
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1301_AlreadyDefinedPCH( const BFFIterator & iter, 
													const Function * function,
													const char * pch )
{
	FormatError( iter, 1301u, function, "Precompiled Header target '%s' has already been defined.",
									   pch );
}

// Error_1302_MissingPCHCompilerOption
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1302_MissingPCHCompilerOption( const BFFIterator & iter, 
															const Function * function,
															const char * option,
															const char * property )
{
	FormatError( iter, 1302u, function, "Missing Precompiled Header option '%s' in '%s'.", option, property );
}

// Error_1303_PCHCreateOptionOnlyAllowedOnPCH
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1303_PCHCreateOptionOnlyAllowedOnPCH( const BFFIterator & iter, 
																   const Function * function,
																   const char * option,
																   const char * property )
{
	FormatError( iter, 1303u, function, "Precompiled Header option '%s' in '%s' invalid. Only allowed on Precompiled Header.", option, property );
}

// FormatError
//------------------------------------------------------------------------------
void Error::FormatError( const BFFIterator & iter,
						 uint32_t errNum, 
						 const Function * function, 
						 const char * message, ... )
{
	ASSERT( message );
	AStackString< 4096 > buffer;

	va_list args;
	va_start(args, message);
	buffer.VFormat( message, args );
	va_end( args );

	// get human readable info about the position of the error
	uint32_t line = 0;
	uint32_t column = 0;
	const char * lineStart = nullptr;
	iter.GetPosInfo( line, column, lineStart );

	// convert to full path and '/'->'\' cleanup
	const AStackString<> fileName( iter.GetFileName() );
	AStackString<> fullPath;
	NodeGraph::CleanPath( fileName, fullPath );

	// deliberately using OUTPUT here to avoid "Error:" in front
	OUTPUT( "%s(%u,%u): FASTBuild Error #%04u - %s%s%s\n",
			fullPath.Get(), 
			line, 
			column, 
			errNum, 
			function ? function->GetName().Get() : "",
			function ? "() - " : "",
			buffer.Get() );

	// find the line end
	BFFIterator lineEnd( iter );
	while ( !lineEnd.IsAtEnd() )
	{
		if (( *lineEnd != '\r' ) && ( *lineEnd != '\n' ))
		{
			lineEnd++;
			continue;
		}
		break;
	}

	// if line is too crazy to be useful, don't print anything more
	size_t lineLength = lineEnd.GetCurrent() - lineStart;
	if ( lineLength >= 256 )
	{
		return;
	}

	// print the problematic line
	AString::Copy( lineStart, buffer.Get(), lineLength );
	FLOG_ERROR( "%s", buffer.Get() );

	// point to the specific pos where parsing broke
	// (taking into account tabs)
	char * c = buffer.Get();
	const char * end = c + column - 1;
	for ( ; c < end; ++c )
	{
		if ( *c != '\t' )
		{
			*c = ' ';
		}
	}

	AString::Copy( "^", c, 1 );
	FLOG_ERROR( buffer.Get() );
	AString::Copy( "\\--here", c, 8 );
	FLOG_ERROR( buffer.Get() );
}

//------------------------------------------------------------------------------
