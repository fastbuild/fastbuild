// FBuiltTest.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FBuildOptions.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"

#include "Core/Process/Mutex.h"

// Forward Declarations
//------------------------------------------------------------------------------
struct FBuildStats;

// FBuildTest
//------------------------------------------------------------------------------
class FBuildTest : public UnitTest
{
protected:
    FBuildTest();

    virtual void PreTest() const override;
    virtual void PostTest( bool pased ) const override;

    // helpers to manage generated files
    void EnsureFileDoesNotExist( const char * fileName ) const;
    void EnsureFileDoesNotExist( const AString & fileName ) const { EnsureFileDoesNotExist( fileName.Get() ); }
    void EnsureFileExists( const char * fileName ) const;
    void EnsureFileExists( const AString & fileName ) const { EnsureFileExists( fileName.Get() ); }
    void EnsureDirDoesNotExist( const char * dirPath ) const;
    void EnsureDirDoesNotExist( const AString & dirPath ) const { EnsureDirDoesNotExist( dirPath.Get() ); }
    void EnsureDirExists( const char * dirPath ) const;
    void EnsureDirExists( const AString & dirPath ) const { EnsureDirExists( dirPath.Get() ); }
    void LoadFileContentsAsString( const char * fileName, AString & outString ) const;
    void MakeFile( const char * fileName, const char * fileContents ) const;

    // Helpers to invoke builds or parse bff files
    void Parse( const char * fileName, bool expectFailure = false ) const;
    bool ParseFromString( bool expectResult,
                          const char * bffContents,
                          const char * expectedMessage = nullptr ) const;

    // Helper macros
    #define TEST_PARSE_OK( ... )        TEST_ASSERT( ParseFromString( true, __VA_ARGS__ ) )
    #define TEST_PARSE_FAIL( ... )      TEST_ASSERT( ParseFromString( false, __VA_ARGS__ ) )

    // Helpers to check build results
    void CheckStatsNode( const FBuildStats & stats, size_t numSeen, size_t numBuilt, Node::Type nodeType ) const;
    void CheckStatsTotal( const FBuildStats & stats, size_t numSeen, size_t numBuilt ) const;
    void CheckStatsNode( size_t numSeen, size_t numBuilt, Node::Type nodeType ) const;
    void CheckStatsTotal( size_t numSeen, size_t numBuilt ) const;

    // other helpers
    static void GetCodeDir( AString & codeDir );

    const AString & GetRecordedOutput() const { return s_RecordedOutput; }

private:
    mutable AString m_OriginalWorkingDir;
    static bool s_DebuggerAttached;
    static bool LoggingCallback( const char * message );
    static Mutex s_OutputMutex;
    static AString s_RecordedOutput;
};

// FBuildTestOptions
//------------------------------------------------------------------------------
class FBuildTestOptions : public FBuildOptions
{
public:
    FBuildTestOptions();
};

// FBuildForTest
//  - Wrapper for tests to use FBuild while accessing some internals for
//    testing purposes
//------------------------------------------------------------------------------
class FBuildForTest : public FBuild
{
public:
    FBuildForTest( FBuildOptions & options )
        : FBuild( options ) {}

    size_t GetRecursiveDependencyCount( const Node * node ) const;
    size_t GetRecursiveDependencyCount( const char * nodeName ) const;

    void GetNodesOfType( Node::Type type, Array<const Node*> & outNodes ) const;
    const Node * GetNode( const char * nodeName ) const;

    void SerializeDepGraphToText( const char * nodeName, AString & outBuffer ) const;
};

//------------------------------------------------------------------------------
