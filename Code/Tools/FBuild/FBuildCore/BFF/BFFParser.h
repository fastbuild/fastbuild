// BFFParser - loads BFF files, extracting build rules
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFToken.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenizer.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFTokenRange;
class BFFUserFunction;
class FileStream;
class Function;
class NodeGraph;

// BFFParser
//------------------------------------------------------------------------------
class BFFParser
{
public:
    explicit BFFParser( NodeGraph & nodeGraph );
    ~BFFParser();

    // Parse BFF data
    bool ParseFromFile( const char * fileName );
    bool ParseFromString( const char * fileName, const char * fileContents );
    bool Parse( BFFTokenRange & tokenRange );

    const Array<BFFFile *> & GetUsedFiles() const { return m_Tokenizer.GetUsedFiles(); }

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
    enum { MAX_OPERATOR_HISTORY = 256 };

    static bool PerformVariableSubstitutions( const BFFToken * inputToken, AString & value );
    static bool ParseVariableName( const BFFToken * iter, AString & name, bool & parentScope );

private:
    bool ParseUnnamedVariableModification( BFFTokenRange & iter );
    bool ParseNamedVariableDeclaration( BFFTokenRange & iter );
    bool ParseVariableDeclaration( BFFTokenRange & iter, const AString & varName, BFFStackFrame * frame );
    bool ParseFunction( BFFTokenRange & iter );
    bool ParseUnnamedScope( BFFTokenRange & iter );
    bool ParseUserFunctionDeclaration( BFFTokenRange & iter );
    bool ParseUserFunctionCall( BFFTokenRange & iter, const BFFUserFunction & function );

    bool FindBracedRange( BFFTokenRange & iter, BFFTokenRange & outBracedRange, const Function * function = nullptr ) const;
    bool FindBracedRangeRecurse( BFFTokenRange & iter ) const;

    bool StoreVariableString( const AString & name, const BFFToken * rhsString, const BFFToken * operatorToken, BFFStackFrame * frame );
    bool StoreVariableArray( const AString & name, const BFFTokenRange & tokenRange, const BFFToken * operatorIter, BFFStackFrame * frame );
    bool StoreVariableStruct( const AString & name, const BFFTokenRange & tokenRange, const BFFToken * operatorToken, BFFStackFrame * frame );
    bool StoreVariableBool( const AString & name, bool value, BFFStackFrame * frame );
    bool StoreVariableInt( const AString & name, int value, BFFStackFrame * frame );
    bool StoreVariableToVariable( const AString & dstName, const BFFToken * rhsToken, const BFFToken * operatorToken, BFFStackFrame * dstFrame );

    void CreateBuiltInVariables();
    void SetBuiltInVariable_CurrentBFFDir( const char * fileName );
    BFFUserFunction * GetUserFunction( const AString & name );

    NodeGraph & m_NodeGraph;

    BFFStackFrame m_BaseStackFrame;

    // CurrentBFFDir related
    AString m_CurrentBFFDir;

    BFFTokenizer m_Tokenizer;

    BFFParser & operator = (const BFFParser &) = delete;
};

//------------------------------------------------------------------------------
