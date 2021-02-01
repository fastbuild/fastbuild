// Error
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

// Core
#include "Core/Env/MSVCStaticAnalysis.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class BFFToken;
class Function;

// FLog class
//------------------------------------------------------------------------------
class Error
{
public:
    //
    // 1000 - 1049 : General parsing errors
    //------------------------------------------------------------------------------
    static void Error_1001_MissingStringStartToken( const BFFToken  * iter,
                                                 const Function * function );
    static void Error_1002_MatchingClosingTokenNotFound( const BFFToken  * iter,
                                                         const Function * function,
                                                         char expectedChar );
    static void Error_1003_EmptyStringNotAllowedInHeader( const BFFToken * iter,
                                                       const Function * function );
    static void Error_1004_EmptyStringPropertyNotAllowed( const BFFToken * iter,
                                                       const Function * function,
                                                       const char * propertyName );
    static void Error_1005_UnsupportedNodeType( const BFFToken * iter,
                                             const Function * function,
                                             const char * propertyName,
                                             const AString & nodeName,
                                             Node::Type nodeType );
    static void Error_1006_NothingToBuild( const BFFToken * iter,
                                        const Function * function );
    static void Error_1007_ExpectedVariable( const BFFToken * iter,
                                             const Function * function );
    static void Error_1008_VariableOfWrongType( const BFFToken * iter,
                                                const Function * function,
                                                BFFVariable::VarType expectedType,
                                                BFFVariable::VarType foundType );
    static void Error_1009_UnknownVariable( const BFFToken * iter,
                                            const Function * function,
                                            const AString & unknownVariableName );
    static void Error_1010_UnknownConstruct( const BFFToken * iter );
    static void Error_1011_UnnamedModifcationMustFollowAssignment( const BFFToken * iter );
    static void Error_1012_UnexpectedEndOfFile( const BFFToken * iter );
    static void Error_1013_UnexpectedCharInVariableName( const BFFToken * iter,
                                                       const Function * function );
    static void Error_1014_VariableNameIsTooLong( const BFFToken * iter,
                                                uint32_t length,
                                                uint32_t maxLength );
    static void Error_1015_UnknownFunction( const BFFToken * iter );
    static void Error_1016_UnexepectedCharFollowingVariableName( const BFFToken * iter );
    static void Error_1017_UnexepectedCharInVariableValue( const BFFToken * iter );
    static void Error_1018_IntegerValueCouldNotBeParsed( const BFFToken * iter );
    static void Error_1020_FunctionCanOnlyBeInvokedOnce( const BFFToken * iter,
                                                       const Function * function );
    static void Error_1021_UnexpectedHeaderForFunction( const BFFToken * iter,
                                                      const Function * function );
    static void Error_1022_MissingFunctionHeaderCloseToken( const BFFToken * iter,
                                                          const Function * function );
    static void Error_1023_FunctionRequiresAHeader( const BFFToken * iter,
                                                  const Function * function );
    static void Error_1024_FunctionRequiresABody( const BFFToken * iter,
                                                const Function * function );
    static void Error_1025_MissingScopeCloseToken( const BFFToken * iter,
                                                   const Function * function );
    static void Error_1026_VariableNotFoundForModification( const BFFToken * iter,
                                                           const AString & variableName );
    static void Error_1027_CannotModify( const BFFToken * iter,
                                            const AString & variableName,
                                            BFFVariable::VarType dstType,
                                            BFFVariable::VarType srcType );
    static void Error_1028_MissingVariableSubstitutionEnd( const BFFToken * iter );
    static void Error_1029_VariableForSubstitutionIsNotAString( const BFFToken * iter,
                                                              const AString & variableName,
                                                              BFFVariable::VarType varType );
    static void Error_1030_UnknownDirective( const BFFToken * iter,
                                             const AString & directiveName );
    static void Error_1031_UnexpectedCharFollowingDirectiveName( const BFFToken * iter,
                                                                 const char * directiveName,
                                                                 char expectedChar );
    static void Error_1032_UnableToOpenInclude( const BFFToken * iter,
                                                const AString & include );
    static void Error_1033_ErrorReadingInclude( const BFFToken * iter,
                                                const AString & include,
                                                uint32_t errorCode );
    static void Error_1034_OperationNotSupported( const BFFToken * rhsIter,
                                                  BFFVariable::VarType lhs,
                                                  BFFVariable::VarType rhs,
                                                  const  BFFToken * operatorIter );
    static void Error_1035_ExcessiveDepthComplexity( const BFFToken * iter );
    //static void Error_1036_UnknownTokenInIfDirective( const BFFToken * iter );
    static void Error_1037_EndIfWithoutIf( const BFFToken * iter );
    static void Error_1038_OverwritingTokenInDefine( const BFFToken * iter );
    static void Error_1039_UnknownTokenInUndef( const BFFToken * iter );
    static void Error_1040_UndefOfBuiltInTokenNotAllowed( const BFFToken * iter );
    static void Error_1041_ElseWithoutIf( const BFFToken * iter );
    static void Error_1042_UnknownOperator(const BFFToken * iter, const AString & operatorName );
    static void Error_1043_CyclicDependencyDetected( const BFFToken * iter, const AString & nodeName );
    static void Error_1044_OperatorExpected( const BFFToken * iter );
    static void Error_1045_ExtraneousTokenFollowingDirective( const BFFToken * iter, const char * directiveName );
    static void Error_1046_IfExpressionCannotStartWithBooleanOperator( const BFFToken* iter );
    static void Error_1047_IfExpressionTooComplex( const BFFToken* iter );

    // 1050 - 1099 : Variable type errors
    //------------------------------------------------------------------------------
    static void Error_1050_PropertyMustBeOfType( const BFFToken * iter,
                                               const Function * function,
                                               const char * propertyName,
                                               BFFVariable::VarType foundType,
                                               BFFVariable::VarType requiredType,
                                               BFFVariable::VarType alternateRequiredType = BFFVariable::VAR_ANY );
    static void Error_1054_IntegerOutOfRange( const BFFToken * iter,
                                            const Function * function,
                                            const char * propertyName,
                                            int rangeMin,
                                            int rangeMax );
    static void Error_1060_CantModifyFrozenVar( const BFFToken * iter,
                                                const Function * function,
                                                const BFFVariable * var );
    static void Error_1070_UnexpectedOperator( const BFFToken * iter,
                                               const Function * function );
    static void Error_1071_UnexpectedToken( const BFFToken * iter,
                                            const Function * function );

    //
    // 1100 - 1199 : General function definition errors
    //------------------------------------------------------------------------------
    static void Error_1100_AlreadyDefined( const BFFToken * iter,
                                          const Function * function,
                                          const AString & name );
    static void Error_1101_MissingProperty( const BFFToken * iter,
                                           const Function * function,
                                           const AString & name );
    static void Error_1102_UnexpectedType( const BFFToken * iter,
                                          const Function * function,
                                          const char * propertyName,
                                          const AString & name,
                                          Node::Type foundType,
                                          Node::Type expectedType );
    static void Error_1103_NotAFile( const BFFToken * iter,
                                    const Function * function,
                                    const char * propertyName,
                                    const AString & name,
                                    Node::Type foundType );
    static void Error_1104_TargetNotDefined( const BFFToken * iter,
                                            const Function * function,
                                            const char * propertyName,
                                            const AString & name );
    static void Error_1105_PathNotAllowed( const BFFToken * iter,
                                           const Function * function,
                                           const char * propertyName,
                                           const AString & propertyValue );
    static void Error_1106_MissingRequiredToken( const BFFToken * iter,
                                                 const Function * function,
                                                 const char * propertyName,
                                                 const char * token );
    static void Error_1107_ExpectedFunctionNameFollowingFunctionKeyword( const BFFToken * iter );
    static void Error_1108_FunctionAlreadyDefined( const BFFToken * iter );
    static void Error_1109_FunctionArgumentAlreadyDefined( const BFFToken * iter );
    static void Error_1110_ExpectedArgumentBlockForFunctionCall( const BFFToken * iter );
    static void Error_1111_FunctionCallArgumentMismatch( const BFFToken * iter,
                                                         uint32_t numArgsProvided,
                                                         uint32_t numArgsExpected );
    static void Error_1112_FunctionCallExpectedArgument( const BFFToken * iter );

    // 1200 - 1299 : ForEach specific errors
    //------------------------------------------------------------------------------
    static void Error_1200_ExpectedVar( const BFFToken * iter,
                                       const Function * function );
    static void Error_1201_MissingIn( const BFFToken * iter,
                                     const Function * function );
    static void Error_1202_ExpectedVarFollowingIn( const BFFToken * iter,
                                                  const Function * function );
    static void Error_1204_LoopVariableLengthsDiffer( const BFFToken * iter,
                                                     const Function * function,
                                                     const char * variableName,
                                                     uint32_t foundSize,
                                                     uint32_t expectedSize );
    static void Error_1254_UnrecognizedOperator( const BFFToken * iter,
                                                 const Function * function );

    // 1300 - 1399 : Library specific errors
    //------------------------------------------------------------------------------
    static void Error_1300_MissingPCHArgs( const BFFToken * iter,
                                          const Function * function );
    static void Error_1301_AlreadyDefinedPCH( const BFFToken * iter,
                                          const Function * function,
                                          const char * pch );
    static void Error_1302_MissingPCHCompilerOption( const BFFToken * iter,
                                                     const Function * function,
                                                     const char * option,
                                                     const char * property );
    static void Error_1303_PCHCreateOptionOnlyAllowedOnPCH( const BFFToken * iter,
                                                            const Function * function,
                                                            const char * option,
                                                            const char * property );

    // 1400-1499 : Copy specific errors
    //------------------------------------------------------------------------------
    static void Error_1400_CopyDestMissingSlash( const BFFToken * iter,
                                                 const Function * function );

    // 1500-1599 : Compiler specific errors
    //------------------------------------------------------------------------------
    static void Error_1500_CompilerDetectionFailed( const BFFToken * iter,
                                                    const Function * function,
                                                    const AString & exe );
    static void Error_1501_CompilerFamilyUnrecognized( const BFFToken * iter,
                                                       const Function * function,
                                                       const AString & badCompilerFamily );
    static void Error_1502_LightCacheIncompatibleWithCompiler( const BFFToken * iter,
                                                               const Function * function );
    static void Error_1503_CSharpCompilerShouldUseCSAssembly( const BFFToken * iter,
                                                              const Function * function );
    static void Error_1504_CSAssemblyRequiresACSharpCompiler( const BFFToken * iter,
                                                              const Function * function );

    // 1900-1999 : User-generate errors
    //------------------------------------------------------------------------------
    static void Error_1999_UserError( const BFFToken * iter,
                                      const Function * function,
                                      const AString & errorMessage );

private:
    static void GetChar( const BFFToken * iter, AString & outBuffer );

    static void FormatError( const BFFToken * iter,
                             uint32_t errNum,
                             const Function * function,
                             MSVC_SAL_PRINTF const char * message, ... ) FORMAT_STRING( 4, 5 );
};

//------------------------------------------------------------------------------
