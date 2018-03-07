// TestBuildAndLinkLibrary.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

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
        const char * codePos = codeDir.FindI( "\\code\\" );
    #else
        const char * codePos = codeDir.FindI( "/code/" );
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
}

//------------------------------------------------------------------------------
