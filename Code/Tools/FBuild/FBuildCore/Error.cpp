// Error
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Error.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFToken.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/NodeGraph.h"

#include "Core/Env/ErrorFormat.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

#include <stdarg.h>
#include <stdio.h>

// Error_1001_MissingStringStartToken
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1001_MissingStringStartToken( const BFFToken * iter,
                                                        const Function * function )
{
    FormatError( iter, 1001u, function, "Missing string start token \" or '." );
}

// Error_1002_MatchingClosingTokenNotFound
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1002_MatchingClosingTokenNotFound( const BFFToken * iter,
                                                                const Function * function,
                                                                char expectedChar )
{
    FormatError( iter, 1002u, function, "Matching closing token %c not found.",
                                     expectedChar );
}

// Error_1003_EmptyStringNotAllowedInHeader
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1003_EmptyStringNotAllowedInHeader( const BFFToken * iter,
                                                                 const Function * function )
{
    FormatError( iter, 1003u, function, "Empty string not allowed in Function header." );
}

// Error_1004_EmptyStringPropertyNotAllowed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1004_EmptyStringPropertyNotAllowed( const BFFToken * iter,
                                                              const Function * function,
                                                              const char * propertyName )
{
    FormatError( iter, 1004u, function, "Empty string not allowed for property '%s'.",
                                     propertyName );
}

// Error_1005_UnsupportedNodeType
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1005_UnsupportedNodeType( const BFFToken * iter,
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
/*static*/ void Error::Error_1006_NothingToBuild( const BFFToken * iter,
                                               const Function * function )
{
    FormatError( iter, 1006u, function, "Nothing defined to be built." );
}

// Error_1007_ExpectedVariable
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1007_ExpectedVariable( const BFFToken * iter,
                                                    const Function * function )
{
    FormatError( iter, 1007u, function, "Expected a variable at this location." );
}

// Error_1008_VariableOfWrongType
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1008_VariableOfWrongType( const BFFToken * iter,
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
/*static*/ void Error::Error_1009_UnknownVariable( const BFFToken * iter,
                                                   const Function * function,
                                                   const AString & unknownVariableName )
{
    FormatError( iter, 1009u, function, "Unknown variable '%s'.", unknownVariableName.Get() );
}

// Error_1010_UnknownConstruct
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1010_UnknownConstruct( const BFFToken * iter )
{
    FormatError( iter, 1010u, nullptr, "Unknown construct." );
}

// Error_1011_UnnamedModifcationMustFollowAssignment
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1011_UnnamedModifcationMustFollowAssignment( const BFFToken * iter )
{
    FormatError( iter, 1011u, nullptr, "Unnamed modification must follow a variable assignment in the same scope." );
}

// Error_1012_UnexpectedEndOfFile
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1012_UnexpectedEndOfFile( const BFFToken * iter )
{
    FormatError( iter, 1012u, nullptr, "Unexpected end of file." );
}

// Error_1013_UnexpectedCharInVariableName
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1013_UnexpectedCharInVariableName( const BFFToken * iter,
                                                              const Function * function )
{
    AStackString<> buffer;
    GetChar( iter, buffer );
    FormatError( iter, 1013u, function, "Unexpected character '%s' in variable name.", buffer.Get() );
}

// Error_1014_VariableNameIsTooLong
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1014_VariableNameIsTooLong( const BFFToken * iter, uint32_t length, uint32_t maxLength )
{
    FormatError( iter, 1014u, nullptr, "Variable name is too long. (Length: %u, MaxLength: %u)", length, maxLength );
}

// Error_1015_UnknownFunction
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1015_UnknownFunction( const BFFToken * iter )
{
    FormatError( iter, 1015u, nullptr, "Unknown Function." );
}

// Error_1016_UnexepectedCharFollowingVariableName
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1016_UnexepectedCharFollowingVariableName( const BFFToken * iter )
{
    AStackString<> buffer;
    GetChar( iter, buffer );
    FormatError( iter, 1016u, nullptr, "Unexpected character '%s' following variable name. (Expected operator)", buffer.Get() );
}

// Error_1017_UnexepectedCharInVariableValue
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1017_UnexepectedCharInVariableValue( const BFFToken * iter )
{
    AStackString<> buffer;
    GetChar( iter, buffer );
    FormatError( iter, 1017u, nullptr, "Unexpected character '%s' in variable value.", buffer.Get() );
}

// Error_1018_IntegerDeclarationCouldNotBeParsed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1018_IntegerValueCouldNotBeParsed( const BFFToken * iter )
{
    FormatError( iter, 1018u, nullptr, "Integer value could not be parsed." );
}

// Error_1020_FunctionCanOnlyBeInvokedOnce
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1020_FunctionCanOnlyBeInvokedOnce( const BFFToken * iter, const Function * function )
{
    FormatError( iter, 1020u, function, "Function can only be invoked once." );
}

// Error_1021_UnexpectedHeaderForFunction
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1021_UnexpectedHeaderForFunction( const BFFToken * iter, const Function * function )
{
    FormatError( iter, 1021u, function, "Unexpected Function header." );
}

// Error_1022_MissingFunctionHeaderCloseToken
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1022_MissingFunctionHeaderCloseToken( const BFFToken * iter, const Function * function )
{
    FormatError( iter, 1022u, function, "Missing Function header close token '%c'.", BFFParser::BFF_FUNCTION_ARGS_CLOSE );
}

// Error_1023_FunctionRequiresAHeader
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1023_FunctionRequiresAHeader( const BFFToken * iter,
                                                         const Function * function )
{
    FormatError( iter, 1023u, function, "Function requires a header." );
}

// Error_1024_FunctionRequiresABody
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1024_FunctionRequiresABody( const BFFToken * iter,
                                                       const Function * function )
{
    FormatError( iter, 1024u, function, "Function requires a body." );
}

// Error_1025_MissingScopeCloseToken
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1025_MissingScopeCloseToken( const BFFToken * iter,
                                                          const Function * function )
{
    FormatError( iter, 1025u, function, "Missing scope close token '%c'.", BFFParser::BFF_SCOPE_CLOSE );
}

// Error_1026_VariableNotFoundForModification
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1026_VariableNotFoundForModification( const BFFToken * iter,
                                                                  const AString & variableName )
{
    FormatError( iter, 1026u, nullptr, "Variable '%s' not found for modification.", variableName.Get() );
}

// Error_1027_CannotModify
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1027_CannotModify( const BFFToken * iter,
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
/*static*/ void Error::Error_1028_MissingVariableSubstitutionEnd( const BFFToken * iter )
{
    FormatError( iter, 1028u, nullptr, "Cannot find '$' to end variable substitution." );
}

// Error_1029_VariableForSubstitutionIsNotAString
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1029_VariableForSubstitutionIsNotAString( const BFFToken * iter,
                                                                     const AString & variableName,
                                                                     BFFVariable::VarType varType )
{
    FormatError( iter, 1029u, nullptr, "Variable for substitution '%s' is not a string (Type: <%s>).",
                                     variableName.Get(),
                                     BFFVariable::GetTypeName( varType ) );
}

// Error_1030_UnknownDirective
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1030_UnknownDirective( const BFFToken * iter,
                                                    const AString & directiveName )
{
    FormatError( iter, 1030u, nullptr, "Unknown directive '%s'.",
                                     directiveName.Get() );
}

// Error_1031_UnexpectedCharFollowingDirectiveName
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1031_UnexpectedCharFollowingDirectiveName( const BFFToken * iter,
                                                                        const char * directiveName,
                                                                        char expectedChar )
{
    AStackString<> buffer;
    GetChar( iter, buffer );
    FormatError( iter, 1031u, nullptr, "Unknown char '%s' following '%s' directive. (Expected '%c').",
                                     buffer.Get(),
                                     directiveName,
                                     expectedChar );
}

// Error_1032_UnableToOpenInclude
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1032_UnableToOpenInclude( const BFFToken * iter,
                                                       const AString & include )
{
    FormatError( iter, 1032u, nullptr, "Unable to open include '%s'.",
                                     include.Get() );
}

// Error_1033_ErrorReadingInclude
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1033_ErrorReadingInclude( const BFFToken * iter,
                                                       const AString & include,
                                                       uint32_t errorCode )
{
    FormatError( iter, 1033u, nullptr, "Error reading include. Error: %s File: '%s'",
                                       ERROR_STR( errorCode ),
                                       include.Get() );
}

// Error_1034_OperationNotSupported
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1034_OperationNotSupported( const BFFToken * rhsIter,
                                                         BFFVariable::VarType lhs,
                                                         BFFVariable::VarType rhs,
                                                         const  BFFToken * operatorIter )
{
    FormatError( rhsIter, 1034u, nullptr, "Operation not supported: '%s' %s '%s'.",
                                       BFFVariable::GetTypeName( lhs ),
                                       operatorIter ? operatorIter->GetValueString().Get() : "???",
                                       BFFVariable::GetTypeName( rhs ) );
}

// Error_1035_ExcessiveDepthComplexity
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1035_ExcessiveDepthComplexity( const BFFToken * iter )
{
    FormatError( iter, 1035u, nullptr, "Excessive depth complexity when parsing BFFs. Check for resursive includes or simplify config." );
}

// Error_1036_UnknownTokenInIfDirective
//------------------------------------------------------------------------------
///*static*/ void Error::Error_1036_UnknownTokenInIfDirective( const BFFToken * iter )
//{
//  FormatError( iter, 1036u, nullptr, "Unknown token in #if directive." );
//}

// Error_1037_EndIfWithoutIf
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1037_EndIfWithoutIf( const BFFToken * iter )
{
    FormatError( iter, 1037u, nullptr, "#endif without matching #if." );
}

// Error_1038_OverwritingTokenInDefine
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1038_OverwritingTokenInDefine( const BFFToken * iter )
{
    FormatError( iter, 1038u, nullptr, "Overwriting already defined token in #define directive." );
}

// Error_1039_UnknownTokenInUndef
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1039_UnknownTokenInUndef( const BFFToken * iter )
{
    FormatError( iter, 1039u, nullptr, "Unknown token in #undef directive." );
}

// Error_1040_UndefOfBuiltInTokenNotAllowed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1040_UndefOfBuiltInTokenNotAllowed( const BFFToken * iter )
{
    FormatError( iter, 1040u, nullptr, "#undef of built-in token not allowed." );
}

// Error_1041_ElseWithoutIf
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1041_ElseWithoutIf( const BFFToken * iter )
{
    FormatError( iter, 1041u, nullptr, "#else without matching #if." );
}

// Error_1042_UnknownOperator
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1042_UnknownOperator( const BFFToken * iter, const AString & operatorName )
{
    if ( operatorName.IsEmpty() )
    {
        FormatError( iter, 1042u, nullptr, "Unknown operator." );
    }
    else
    {
        FormatError( iter, 1042u, nullptr, "Unknown operator '%s'.", operatorName.Get() );
    }
}

// Error_1043_CyclicDependencyDetected
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1043_CyclicDependencyDetected( const BFFToken * iter, const AString & nodeName )
{
    FormatError( iter, 1043u, nullptr, "Cyclic dependency detected for node '%s'.", nodeName.Get() );
}

// Error_1044_OperatorExpected
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1044_OperatorExpected( const BFFToken * iter )
{
    FormatError( iter, 1044u, nullptr, "Operator expected." );
}

// Error_1045_UnknownTokenFollowingDirective
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1045_ExtraneousTokenFollowingDirective( const BFFToken * iter, const char * directiveName )
{
    FormatError( iter, 1045u, nullptr, "Extraneous token(s) following '%s' directive.", directiveName );
}

// Error_1046_IfExpressionCannotStartWithBooleanOperator
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1046_IfExpressionCannotStartWithBooleanOperator( const BFFToken * iter )
{
    FormatError( iter, 1046u, nullptr, "#if expression cannot start with boolean operator." );
}

// Error_1047_IfExpressionTooComplex
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1047_IfExpressionTooComplex( const BFFToken* iter )
{
    FormatError( iter, 1047u, nullptr, "If expression too complex. Up to %i boolean operators supported.", BFFParser::MAX_OPERATOR_HISTORY );
}

// Error_1050_PropertyMustBeString
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1050_PropertyMustBeOfType( const BFFToken * iter,
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
/*static*/ void Error::Error_1054_IntegerOutOfRange( const BFFToken * iter,
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
/*static*/ void Error::Error_1060_CantModifyFrozenVar( const BFFToken * iter, const Function * function, const BFFVariable * var )
{
    FormatError( iter, 1060u, function, "Can't modify frozen variable '%s'", var->GetName().Get() );
}

// Error_1100_AlreadyDefined
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1100_AlreadyDefined( const BFFToken * iter,
                                                 const Function * function,
                                                 const AString & name )
{
    FormatError( iter, 1100u, function, "Target '%s' already defined.",
                                       name.Get() );
}

// Error_1101_MissingProperty
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1101_MissingProperty( const BFFToken * iter,
                                                  const Function * function,
                                                  const AString & name )
{
    FormatError( iter, 1101u, function, "Missing required property '%s'.",
                                       name.Get() );
}

// Error_1102_UnexpectedType
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1102_UnexpectedType( const BFFToken * iter,
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
/*static*/ void Error::Error_1103_NotAFile( const BFFToken * iter,
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
/*static*/ void Error::Error_1104_TargetNotDefined( const BFFToken * iter,
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
/*static*/ void Error::Error_1105_PathNotAllowed( const BFFToken * iter,
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
/*static*/ void Error::Error_1106_MissingRequiredToken( const BFFToken * iter,
                                                        const Function * function,
                                                        const char * propertyName,
                                                        const char * token )
{
    FormatError( iter, 1106u, function, "Property '%s' is missing required token '%s'.",
                                        propertyName,
                                        token );
}

// Error_1107_ExpectedFunctionNameFollowingFunctionKeyword
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1107_ExpectedFunctionNameFollowingFunctionKeyword( const BFFToken * iter )
{
    FormatError( iter, 1107u, nullptr, "Expected function name following 'function' keyword." );
}

// Error_1108_FunctionAlreadyDefined
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1108_FunctionAlreadyDefined( const BFFToken * iter )
{
    FormatError( iter, 1108u, nullptr, "Function '%s' already defined.",
                                       iter->GetValueString().Get() );
}

// Error_1109_FunctionArgumentAlreadyDefined
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1109_FunctionArgumentAlreadyDefined( const BFFToken * iter )
{
    FormatError( iter, 1109u, nullptr, "Function argument '%s' is already defined.",
                                       iter->GetValueString().Get() );
}

// Error_1110_ExpectedArgumentBlockForFunctionCall
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1110_ExpectedArgumentBlockForFunctionCall( const BFFToken * iter )
{
    FormatError( iter, 1110u, nullptr, "Expected argument block following function call." );
}

// Error_1111_FunctionCallArgumentMismatch
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1111_FunctionCallArgumentMismatch( const BFFToken * iter,
                                                                uint32_t numArgsProvided,
                                                                uint32_t numArgsExpected )
{
    FormatError( iter, 1111u, nullptr, "Function call does not take %u args (it expects %u args).",
                                       numArgsProvided,
                                       numArgsExpected );
}

// Error_1112_FunctionCallExpectedArgument
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1112_FunctionCallExpectedArgument( const BFFToken * iter )
{
    FormatError( iter, 1112u, nullptr, "Function call arguments should be literals or variables." );
}

// Error_1200_ExpectedVar // TODO:C Remove (Deprecated by 1007)
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1200_ExpectedVar( const BFFToken * iter, const Function * function )
{
    FormatError( iter, 1200u, function, "Expected a variable at this location." );
}

// Error_1201_MissingIn
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1201_MissingIn( const BFFToken * iter, const Function * function )
{
    FormatError( iter, 1201u, function, "Missing 'in' at this location." );
}

// Error_1202_ExpectedVarFollowingIn
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1202_ExpectedVarFollowingIn( const BFFToken * iter, const Function * function )
{
    FormatError( iter, 1202u, function, "Expected a variable following 'in'." );
}

// Error_1204_LoopVariableLengthsDiffer
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1204_LoopVariableLengthsDiffer( const BFFToken * iter,
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

// Error_1254_UnrecognizedOperator
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1254_UnrecognizedOperator( const BFFToken * iter,
                                                        const Function * function )
{
    FormatError( iter, 1254u, function, "Unrecognized operator." );
}

// Error_1300_MissingPCHArgs
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1300_MissingPCHArgs( const BFFToken * iter,
                                                 const Function * function )
{
    FormatError( iter, 1300u, function, "Precompiled Header use requires 'PCHInputFile', 'PCHOutputFile' and 'PCHOptions' parameters." );
}

// Error_1301_AlreadyDefinedPCH
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1301_AlreadyDefinedPCH( const BFFToken * iter,
                                                    const Function * function,
                                                    const char * pch )
{
    FormatError( iter, 1301u, function, "Precompiled Header target '%s' has already been defined.",
                                       pch );
}

// Error_1302_MissingPCHCompilerOption
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1302_MissingPCHCompilerOption( const BFFToken * iter,
                                                            const Function * function,
                                                            const char * option,
                                                            const char * property )
{
    FormatError( iter, 1302u, function, "Missing Precompiled Header option '%s' in '%s'.", option, property );
}

// Error_1303_PCHCreateOptionOnlyAllowedOnPCH
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1303_PCHCreateOptionOnlyAllowedOnPCH( const BFFToken * iter,
                                                                   const Function * function,
                                                                   const char * option,
                                                                   const char * property )
{
    FormatError( iter, 1303u, function, "Precompiled Header option '%s' in '%s' invalid. Only allowed on Precompiled Header.", option, property );
}

// Error_1400_CopyDestMissingSlash
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1400_CopyDestMissingSlash( const BFFToken * iter,
                                                        const Function * function )
{
    FormatError( iter, 1400u, function, "'Dest' with multiple 'Source' files should be a path. Missing trailing '/'?" );
}

// Error_1500_CompilerDetectionFailed
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1500_CompilerDetectionFailed( const BFFToken * iter,
                                                           const Function * function,
                                                           const AString & exe )
{
    FormatError( iter, 1500u, function, "Compiler detection failed. Unrecognized executable '%s'.", exe.Get() );
}

// Error_1501_CompilerFamilyUnrecognized
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1501_CompilerFamilyUnrecognized( const BFFToken * iter,
                                                              const Function * function,
                                                              const AString & badCompilerFamily )
{
    FormatError( iter, 1501u, function, ".CompilerFamily '%s' is unrecognized.", badCompilerFamily.Get() );
}

// Error_1502_LightCacheIncompatibleWithCompiler
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1502_LightCacheIncompatibleWithCompiler( const BFFToken * iter,
                                                                       const Function * function )
{
    FormatError( iter, 1502u, function, "LightCache only compatible with MSVC Compiler." );
}

// Error_1503_CSharpCompilerShouldUseCSAssembly
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1503_CSharpCompilerShouldUseCSAssembly( const BFFToken * iter,
                                                                     const Function * function )
{
    FormatError( iter, 1503u, function, "C# compiler should use CSAssembly." );
}

// Error_1504_CSAssemblyRequiresACSharpCompiler
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1504_CSAssemblyRequiresACSharpCompiler( const BFFToken * iter,
                                                                     const Function * function )
{
    FormatError( iter, 1504u, function, "CSAssembly requires a C# Compiler." );
}

// Error_1999_UserError
//------------------------------------------------------------------------------
/*static*/ void Error::Error_1999_UserError( const BFFToken * iter,
                                             const Function * function,
                                             const AString & errorMessage )
{
    FormatError( iter, 1999u, function, "User Error: %s", errorMessage.Get() );
}

// GetChar
//------------------------------------------------------------------------------
/*static*/ void Error::GetChar( const BFFToken * iter, AString & outBuffer )
{
    // Handle invalid iterators (should only be for for tests)
    if ( ( iter == nullptr ) || ( iter->GetSourcePos() == nullptr ) )
    {
        outBuffer = "?";
        return;
    }

    // Is it a printable character?
    const char printableChars[] = "abcdefghijklmnopqrstuzwxyz"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                  "1234567890"
                                  "~`!@#$%^&*()-_+="
                                   "{[]}|\\"
                                   ";:\"'" 
                                   "<,.>/?";
    bool printable = false;
    const char c = *iter->GetSourcePos();
    for( const char pc : printableChars )
    {
        if ( ( c == pc ) && ( c != 0 ) )
        {
            printable = true;
            break;
        }
    }

    // print char if ok to do so, otherwise print hex representation
    if ( printable )
    {
        outBuffer.Format( "%c", c );
    }
    else
    {
        outBuffer.Format( "0x%02x", c );
    }
}

// FormatError
//------------------------------------------------------------------------------
void Error::FormatError( const BFFToken * iter,
                         uint32_t errNum,
                         const Function * function,
                         MSVC_SAL_PRINTF const char * message, ... )
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
    if ( iter )
    {
        iter->GetPosInfo( line, column, lineStart );
    }

    // convert to full path and '/'->'\' cleanup
    const AStackString<> fileName( iter ? iter->GetSourceFileName().Get() : "Unknown" );
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

    // can't print line if it's not available
    if ( lineStart == nullptr )
    {
        return;
    }

    // find the line end
    const char * lineEnd = lineStart;
    while ( ( *lineEnd != '\r' ) && ( *lineEnd != '\n' ) && ( *lineEnd !=  0 ) )
    {
        lineEnd++;
    }

    // if line is too crazy to be useful, don't print anything more
    const size_t lineLength = (size_t)( lineEnd - lineStart );
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
    FLOG_ERROR( "%s", buffer.Get() );
    AString::Copy( "\\--here", c, 8 );
    FLOG_ERROR( "%s", buffer.Get() );
}

//------------------------------------------------------------------------------
