// Error
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/BFF/BFFVariable.h"

// Forward Declarations
//------------------------------------------------------------------------------
class AString;
class BFFIterator;
class Function;

// FLog class
//------------------------------------------------------------------------------
class Error
{
public:
    //
    // 1000 - 1049 : General parsing errors
    //------------------------------------------------------------------------------
    static void Error_1001_MissingStringStartToken( const BFFIterator & iter,
                                                 const Function * function );
    static void Error_1002_MatchingClosingTokenNotFound( const BFFIterator & iter,
                                                         const Function * function,
                                                         char expectedChar );
    static void Error_1003_EmptyStringNotAllowedInHeader( const BFFIterator & iter,
                                                       const Function * function );
    static void Error_1004_EmptyStringPropertyNotAllowed( const BFFIterator & iter,
                                                       const Function * function,
                                                       const char * propertyName );
    static void Error_1005_UnsupportedNodeType( const BFFIterator & iter,
                                             const Function * function,
                                             const char * propertyName,
                                             const AString & nodeName,
                                             Node::Type nodeType );
    static void Error_1006_NothingToBuild( const BFFIterator & iter,
                                        const Function * function );
    static void Error_1007_ExpectedVariable( const BFFIterator & iter,
                                             const Function * function );
    static void Error_1008_VariableOfWrongType( const BFFIterator & iter,
                                                const Function * function,
                                                BFFVariable::VarType expectedType,
                                                BFFVariable::VarType foundType );
    static void Error_1009_UnknownVariable( const BFFIterator & iter,
                                            const Function * function,
                                            const AString & unknownVariableName );
    static void Error_1010_UnknownConstruct( const BFFIterator & iter );
    static void Error_1011_UnnamedModifcationMustFollowAssignment( const BFFIterator & iter );
    static void Error_1012_UnexpectedEndOfFile( const BFFIterator & iter );
    static void Error_1013_UnexpectedCharInVariableName( const BFFIterator & iter,
                                                       const Function * function );
    static void Error_1014_VariableNameIsTooLong( const BFFIterator & iter,
                                                uint32_t length,
                                                uint32_t maxLength );
    static void Error_1015_UnknownFunction( const BFFIterator & iter );
    static void Error_1016_UnexepectedCharFollowingVariableName( const BFFIterator & iter );
    static void Error_1017_UnexepectedCharInVariableValue( const BFFIterator & iter );
    static void Error_1018_IntegerValueCouldNotBeParsed( const BFFIterator & iter );
    static void Error_1020_FunctionCanOnlyBeInvokedOnce( const BFFIterator & iter,
                                                       const Function * function );
    static void Error_1021_UnexpectedHeaderForFunction( const BFFIterator & iter,
                                                      const Function * function );
    static void Error_1022_MissingFunctionHeaderCloseToken( const BFFIterator & iter,
                                                          const Function * function );
    static void Error_1023_FunctionRequiresAHeader( const BFFIterator & iter,
                                                  const Function * function );
    static void Error_1024_FunctionRequiresABody( const BFFIterator & iter,
                                                const Function * function );
    static void Error_1025_MissingScopeCloseToken( const BFFIterator & iter,
                                                   const Function * function );
    static void Error_1026_VariableNotFoundForModification( const BFFIterator & iter,
                                                           const AString & variableName );
    static void Error_1027_CannotModify( const BFFIterator & iter,
                                            const AString & variableName,
                                            BFFVariable::VarType dstType,
                                            BFFVariable::VarType srcType );
    static void Error_1028_MissingVariableSubstitutionEnd( const BFFIterator & iter );
    static void Error_1029_VariableForSubstitutionIsNotAString( const BFFIterator & iter,
                                                              const AString & variableName,
                                                              BFFVariable::VarType varType );
    static void Error_1030_UnknownDirective( const BFFIterator & iter,
                                             const AString & directiveName );
    static void Error_1031_UnexpectedCharFollowingDirectiveName( const BFFIterator & iter,
                                                                 const AString & directive,
                                                                 char expectedChar );
    static void Error_1032_UnableToOpenInclude( const BFFIterator & iter,
                                                const AString & include );
    static void Error_1033_ErrorReadingInclude( const BFFIterator & iter,
                                                const AString & include,
                                                uint32_t errorCode );
    static void Error_1034_OperationNotSupported( const BFFIterator & rhsIter,
                                                  BFFVariable::VarType lhs,
                                                  BFFVariable::VarType rhs,
                                                  const  BFFIterator & operatorIter );
    static void Error_1035_ExcessiveDepthComplexity( const BFFIterator & iter );
    //static void Error_1036_UnknownTokenInIfDirective( const BFFIterator & iter );
    static void Error_1037_EndIfWithoutIf( const BFFIterator & iter );
    static void Error_1038_OverwritingTokenInDefine( const BFFIterator & iter );
    static void Error_1039_UnknownTokenInUndef( const BFFIterator & iter );
    static void Error_1040_UndefOfBuiltInTokenNotAllowed( const BFFIterator & iter );
    static void Error_1041_ElseWithoutIf( const BFFIterator & iter );
    static void Error_1042_UnknownOperator(const BFFIterator & iter, const AString & operatorName );
    static void Error_1043_CyclicDependencyDetected( const BFFIterator & iter, const AString & nodeName );

    // 1050 - 1099 : Variable type errors
    //------------------------------------------------------------------------------
    static void Error_1050_PropertyMustBeOfType( const BFFIterator & iter,
                                               const Function * function,
                                               const char * propertyName,
                                               BFFVariable::VarType foundType,
                                               BFFVariable::VarType requiredType,
                                               BFFVariable::VarType alternateRequiredType = BFFVariable::VAR_ANY );
    static void Error_1054_IntegerOutOfRange( const BFFIterator & iter,
                                            const Function * function,
                                            const char * propertyName,
                                            int rangeMin,
                                            int rangeMax );
    static void Error_1060_CantModifyFrozenVar( const BFFIterator & iter,
                                                const Function * function,
                                                const BFFVariable * var );

    //
    // 1100 - 1199 : General function definition errors
    //------------------------------------------------------------------------------
    static void Error_1100_AlreadyDefined( const BFFIterator & iter,
                                          const Function * function,
                                          const AString & name );
    static void Error_1101_MissingProperty( const BFFIterator & iter,
                                           const Function * function,
                                           const AString & name );
    static void Error_1102_UnexpectedType( const BFFIterator & iter,
                                          const Function * function,
                                          const char * propertyName,
                                          const AString & name,
                                          Node::Type foundType,
                                          Node::Type expectedType );
    static void Error_1103_NotAFile( const BFFIterator & iter,
                                    const Function * function,
                                    const char * propertyName,
                                    const AString & name,
                                    Node::Type foundType );
    static void Error_1104_TargetNotDefined( const BFFIterator & iter,
                                            const Function * function,
                                            const char * propertyName,
                                            const AString & name );
    static void Error_1105_PathNotAllowed( const BFFIterator & iter,
                                           const Function * function,
                                           const char * propertyName,
                                           const AString & propertyValue );
    static void Error_1106_MissingRequiredToken( const BFFIterator & iter,
                                                 const Function * function,
                                                 const char * propertyName,
                                                 const char * token );

    // 1200 - 1299 : ForEach specific errors
    //------------------------------------------------------------------------------
    static void Error_1200_ExpectedVar( const BFFIterator & iter,
                                       const Function * function );
    static void Error_1201_MissingIn( const BFFIterator & iter,
                                     const Function * function );
    static void Error_1202_ExpectedVarFollowingIn( const BFFIterator & iter,
                                                  const Function * function );
    static void Error_1204_LoopVariableLengthsDiffer( const BFFIterator & iter,
                                                     const Function * function,
                                                     const char * variableName,
                                                     uint32_t foundSize,
                                                     uint32_t expectedSize );
    static void Error_1254_UnrecognizedOperator( const BFFIterator & iter,
                                                 const Function * function );

    // 1300 - 1399 : Library specific errors
    //------------------------------------------------------------------------------
    static void Error_1300_MissingPCHArgs( const BFFIterator & iter,
                                          const Function * function );
    static void Error_1301_AlreadyDefinedPCH( const BFFIterator & iter,
                                          const Function * function,
                                          const char * pch );
    static void Error_1302_MissingPCHCompilerOption( const BFFIterator & iter,
                                                     const Function * function,
                                                     const char * option,
                                                     const char * property );
    static void Error_1303_PCHCreateOptionOnlyAllowedOnPCH( const BFFIterator & iter,
                                                            const Function * function,
                                                            const char * option,
                                                            const char * property );

    // 1400-1499 : Copy specific errors
    //------------------------------------------------------------------------------
    static void Error_1400_CopyDestMissingSlash( const BFFIterator & iter,
                                                 const Function * function );

    // 1500-1599 : Compiler specific errors
    //------------------------------------------------------------------------------
    static void Error_1500_CompilerDetectionFailed( const BFFIterator & iter,
                                                    const Function * function,
                                                    const AString & exe );
    static void Error_1501_CompilerFamilyUnrecognized( const BFFIterator & iter,
                                                       const Function * function,
                                                       const AString & badCompilerFamily );

private:
    static void FormatError( const BFFIterator & iter,
                             uint32_t errNum,
                             const Function * function,
                             const char * message, ... ) FORMAT_STRING( 4, 5 );
};

//------------------------------------------------------------------------------
