// BFFParser - loads BFF files, extracting build rules
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/BFF/BFFStackFrame.h"
#include "Tools/FBuild/FBuildCore/BFF/LinkerNodeFileExistsCache.h"
#include "Tools/FBuild/FBuildCore/BFF/Tokenizer/BFFTokenizer.h"

#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class BFFFile;
class BFFToken;
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

    inline static const char kBFFCommentSemicolon = ';';
    inline static const char kBFFCommentSlash = '/';
    inline static const char kBFFDeclareVarInternal = '.';
    inline static const char kBFFDeclareVarParent = '^';
    inline static const char kBFFVariableAssignment = '=';
    inline static const char kBFFVariableConcatenation = '+';
    inline static const char kBFFVariableSubtraction = '-';
    inline static const char kBFFStartArray = '{';
    inline static const char kBFFEndArray = '}';
    inline static const char kBFFFunctionArgsOpen = '(';
    inline static const char kBFFFunctionArgsClose = ')';
    inline static const char kBFFScopeOpen = '{';
    inline static const char kBFFScopeClose = '}';
    inline static const char kBFFStructOpen = '[';
    inline static const char kBFFStructClose = ']';
    inline static const char kBFFPreprocessorStart = '#';

    inline static const size_t kMaxVariableNameLength = 256;
    inline static const size_t kMaxOperatorHistory = 256;

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
    bool StoreVariableBool( const AString & name, const BFFToken * token, bool value, BFFStackFrame * frame );
    bool StoreVariableInt( const AString & name, const BFFToken * token, int value, BFFStackFrame * frame );
    bool StoreVariableToVariable( const AString & dstName, const BFFToken * rhsToken, const BFFToken * operatorToken, BFFStackFrame * dstFrame );

    void CreateBuiltInVariables();
    void SetBuiltInVariable_CurrentBFFDir( const BFFFile & file );
    BFFUserFunction * GetUserFunction( const AString & name );

    NodeGraph & m_NodeGraph;

    BFFStackFrame m_BaseStackFrame;

    // CurrentBFFDir related
    const BFFFile * m_CurrentBFFFile = nullptr;

    BFFTokenizer m_Tokenizer;
    LinkerNodeFileExistsCache m_LinkerNodeFileExistsCache;

    BFFParser & operator=( const BFFParser & ) = delete;
};

//------------------------------------------------------------------------------
