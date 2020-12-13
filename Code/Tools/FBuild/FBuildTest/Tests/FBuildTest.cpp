// TestBuildAndLinkLibrary.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/BFF/BFFParser.h"
#include "Tools/FBuild/FBuildCore/BFF/Functions/Function.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// Static Data
//------------------------------------------------------------------------------
/*static*/ bool FBuildTest::s_DebuggerAttached( false );
/*static*/ Mutex FBuildTest::s_OutputMutex;
/*static*/ AString FBuildTest::s_RecordedOutput( 1024 * 1024 );

// CONSTRUCTOR (FBuildTest)
//------------------------------------------------------------------------------
FBuildTest::FBuildTest()
{
    s_DebuggerAttached = IsDebuggerAttached();
    m_OriginalWorkingDir.SetReserved( 512 );
}

// PreTest
//------------------------------------------------------------------------------
/*virtual*/ void FBuildTest::PreTest() const
{
    Tracing::AddCallbackOutput( LoggingCallback );
    s_RecordedOutput.Clear();

    FBuildStats::SetIgnoreCompilerNodeDeps( true );

    // Store current working
    VERIFY( FileIO::GetCurrentDir( m_OriginalWorkingDir ) );

    // Set the WorkingDir to be the source code "Code" dir
    AStackString<> codeDir;
    GetCodeDir( codeDir );
    VERIFY( FileIO::SetCurrentDir( codeDir ) );
}

// PostTest
//------------------------------------------------------------------------------
/*virtual*/ void FBuildTest::PostTest( bool passed ) const
{
    VERIFY( FileIO::SetCurrentDir( m_OriginalWorkingDir ) );

    FBuildStats::SetIgnoreCompilerNodeDeps( false );

    Tracing::RemoveCallbackOutput( LoggingCallback );

    // Print the output on failure, unless in the debugger
    // (we print as we go if the debugger is attached)
    if ( ( passed == false ) && ( s_DebuggerAttached == false ) )
    {
        OUTPUT( "%s", s_RecordedOutput.Get() );
    }
}

// EnsureFileDoesNotExist
//------------------------------------------------------------------------------
void FBuildTest::EnsureFileDoesNotExist( const char * fileName ) const
{
    FileIO::FileDelete( fileName );
    TEST_ASSERT( FileIO::FileExists( fileName ) == false );
}

// EnsureFileExists
//------------------------------------------------------------------------------
void FBuildTest::EnsureFileExists( const char * fileName ) const
{
    TEST_ASSERT( FileIO::FileExists( fileName ) );
}

// EnsureDirDoesNotExist
//------------------------------------------------------------------------------
void FBuildTest::EnsureDirDoesNotExist( const char * dirPath ) const
{
    FileIO::DirectoryDelete( AStackString<>( dirPath ) );
    TEST_ASSERT( FileIO::DirectoryExists( AStackString<>( dirPath ) ) == false );
}

// EnsureDirExists
//------------------------------------------------------------------------------
void FBuildTest::EnsureDirExists( const char * dirPath ) const
{
    TEST_ASSERT( FileIO::EnsurePathExists( AStackString<>( dirPath ) ) );
}

// LoadFileContentsAsString
//------------------------------------------------------------------------------
void FBuildTest::LoadFileContentsAsString( const char * fileName, AString & outString ) const
{
    FileStream f;
    TEST_ASSERT( f.Open( fileName ) );
    const uint32_t fileSize = (uint32_t)f.GetFileSize();
    outString.SetLength( fileSize );
    TEST_ASSERT( f.ReadBuffer( outString.Get(), fileSize ) );
}

// MakeFile
//------------------------------------------------------------------------------
void FBuildTest::MakeFile( const char * fileName, const char * fileContents ) const
{
    FileStream f;
    TEST_ASSERT( f.Open( fileName, FileStream::WRITE_ONLY ) );
    const size_t len = AString::StrLen( fileContents );
    TEST_ASSERT( f.WriteBuffer( fileContents, len ) == len );
}

// Parse
//------------------------------------------------------------------------------
void FBuildTest::Parse( const char * fileName, bool expectFailure ) const
{
    FBuild fBuild;
    NodeGraph ng;
    BFFParser p( ng );
    bool parseResult = p.ParseFromFile( fileName );
    if ( expectFailure )
    {
        TEST_ASSERT( parseResult == false ); // Make sure it failed as expected
    }
    else
    {
        TEST_ASSERT( parseResult == true );
    }
}

// ParseFromString
//------------------------------------------------------------------------------
bool FBuildTest::ParseFromString( bool expectedResult,
                                  const char * bffContents,
                                  const char * expectedMessage,
                                  const char * unexpectedMessage ) const
{
    // Note size of output so we can check if message was part of this invocation
    const size_t outputSizeBefore = GetRecordedOutput().GetLength();
    const char * searchStart = GetRecordedOutput().Get() + outputSizeBefore;

    // Parse
    FBuild fBuild;
    NodeGraph ng;
    BFFParser p( ng );
    const bool result = p.ParseFromString( "test.bff", bffContents );

    // Check result is as expected
    if ( result != expectedResult )
    {
        // Emit message about mismatch
        OUTPUT( "Test %s but %s was expected", result ? "succeeded" : "failed",
                                               expectedResult ? "success" : "failure" );
        return false; // break in calling code
    }

    // Check message was present if needed
    if ( expectedMessage )
    {
        // Search for expected message in output
        const bool foundExpectedMessage = ( GetRecordedOutput().Find( expectedMessage, searchStart ) != nullptr );
        if ( foundExpectedMessage == false )
        {
            OUTPUT( "Expected %s was not found: %s", expectedResult ? "message" : "error", expectedMessage );
            return false;
        }
    }

    // Check message was present if unexpected
    if ( unexpectedMessage )
    {
        // Search for unexpected message in output
        const bool foundUnexpectedMessage = ( GetRecordedOutput().Find( unexpectedMessage, searchStart ) != nullptr );
        if ( foundUnexpectedMessage )
        {
            OUTPUT( "Unexpected %s was found: %s", expectedResult ? "message" : "error", unexpectedMessage );
            return false;
        }
    }

    return true;
}

// CheckStatsNode
//------------------------------------------------------------------------------
void FBuildTest::CheckStatsNode( const FBuildStats & stats, size_t numSeen, size_t numBuilt, Node::Type nodeType ) const
{
    const FBuildStats::Stats & nodeStats = stats.GetStatsFor( nodeType );

    const uint32_t actualNumSeen = nodeStats.m_NumProcessed;
    const uint32_t actualNumBuilt = nodeStats.m_NumBuilt;

    const bool nodeStatsOk = ( actualNumSeen == numSeen ) && ( actualNumBuilt == numBuilt );
    if ( !nodeStatsOk )
    {
        if ( actualNumSeen != numSeen )
        {
            OUTPUT( "PROBLEM for %s: Saw %u instead of %u\n", Node::GetTypeName( nodeType ), actualNumSeen, (uint32_t)numSeen );
        }
        if ( actualNumBuilt != numBuilt )
        {
            OUTPUT( "PROBLEM for %s: Built %u instead of %u\n", Node::GetTypeName( nodeType ), actualNumBuilt, (uint32_t)numBuilt );
        }
        TEST_ASSERT( false );
    }
}

// CheckStatsTotal
//------------------------------------------------------------------------------
void FBuildTest::CheckStatsTotal( const FBuildStats & stats, size_t numSeen, size_t numBuilt ) const
{
    size_t actualNumSeen = stats.GetNodesProcessed();
    TEST_ASSERT( actualNumSeen == numSeen );

    size_t actualNumBuilt = stats.GetNodesBuilt();
    TEST_ASSERT( actualNumBuilt == numBuilt );
}

// CheckStatsNode
//------------------------------------------------------------------------------
void FBuildTest::CheckStatsNode( size_t numSeen, size_t numBuilt, Node::Type nodeType ) const
{
    const FBuildStats & stats = FBuild::Get().GetStats();
    CheckStatsNode( stats, numSeen, numBuilt, nodeType );
}

// CheckStatsTotal
//------------------------------------------------------------------------------
void FBuildTest::CheckStatsTotal( size_t numSeen, size_t numBuilt ) const
{
    const FBuildStats & stats = FBuild::Get().GetStats();
    CheckStatsTotal( stats, numSeen, numBuilt );
}

// GetCodeDir
//------------------------------------------------------------------------------
/*static*/ void FBuildTest::GetCodeDir( AString & codeDir )
{
    // we want the working dir to be the 'Code' directory
    TEST_ASSERT( FileIO::GetCurrentDir( codeDir ) );
    if ( !codeDir.EndsWith( NATIVE_SLASH ) )
    {
        codeDir += NATIVE_SLASH;
    }
    #if defined( __WINDOWS__ )
        const char * codePos = codeDir.FindLastI( "\\code\\" );
    #else
        const char * codePos = codeDir.FindLastI( "/code/" );
    #endif
    TEST_ASSERT( codePos );
    codeDir.SetLength( (uint16_t)( codePos - codeDir.Get() + 6 ) );
}

// LoggingCallback
//------------------------------------------------------------------------------
bool FBuildTest::LoggingCallback( const char * message )
{
    MutexHolder mh( s_OutputMutex );
    s_RecordedOutput.Append( message, AString::StrLen( message ) );
    // If in the debugger, print the output normally as well, otherwise
    // suppress and only print on failure
    return s_DebuggerAttached;
}

// CONSTRUCTOR - FBuildTestOptions
//------------------------------------------------------------------------------
FBuildTestOptions::FBuildTestOptions()
{
    // Override defaults
    m_ShowSummary = true; // required to generate stats for node count checks

    // Ensure any distributed compilation tests use the test port
    m_DistributionPort = Protocol::PROTOCOL_TEST_PORT;
}

// GetRecursiveDependencyCount
//------------------------------------------------------------------------------
size_t FBuildForTest::GetRecursiveDependencyCount( const Node * node ) const
{
    size_t count = 0;
    const Dependencies * depLists[ 3 ] = { &node->GetPreBuildDependencies(),
                                           &node->GetStaticDependencies(),
                                           &node->GetDynamicDependencies() };
    for ( const Dependencies * depList : depLists )
    {
        for ( const Dependency & dep : *depList )
        {
            count += GetRecursiveDependencyCount( dep.GetNode() );
        }
        count += depList->GetSize();
    }
    return count;
}

// GetRecursiveDependencyCount
//------------------------------------------------------------------------------
size_t FBuildForTest::GetRecursiveDependencyCount( const char * nodeName ) const
{
    const Node * node = m_DependencyGraph->FindNode( AStackString<>( nodeName ) );
    TEST_ASSERT( node );
    return GetRecursiveDependencyCount( node );
}

// GetNodesOfType
//------------------------------------------------------------------------------
void FBuildForTest::GetNodesOfType( Node::Type type, Array<const Node *> & outNodes ) const
{
    const size_t numNodes = m_DependencyGraph->GetNodeCount();
    for ( size_t i = 0; i < numNodes; ++i )
    {
        const Node * node = m_DependencyGraph->GetNodeByIndex( i );
        if ( node->GetType() == type )
        {
            outNodes.Append( node );
        }
    }
}

// GetNode
//------------------------------------------------------------------------------
const Node * FBuildForTest::GetNode( const char * nodeName ) const
{
    return m_DependencyGraph->FindNode( AStackString<>( nodeName ) );
}

// SerializeDepGraphToText
//------------------------------------------------------------------------------
void FBuildForTest::SerializeDepGraphToText( const char * nodeName, AString & outBuffer ) const
{
    Node * node = m_DependencyGraph->FindNode( AStackString<>( nodeName ) );
    Dependencies deps( 1, false );
    deps.EmplaceBack( node );
    m_DependencyGraph->SerializeToText( deps, outBuffer );
}

//------------------------------------------------------------------------------
