// TestBuildAndLinkLibrary.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildTest.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"

#include "Core/FileIO/FileIO.h"
#include "Core/Strings/AStackString.h"


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

// CheckStatsNode
//------------------------------------------------------------------------------
void FBuildTest::CheckStatsNode( const FBuildStats & stats, size_t numSeen, size_t numBuilt, Node::Type nodeType ) const
{
	const FBuildStats::Stats & nodeStats = stats.GetStatsFor( nodeType );

	size_t actualNumSeen = nodeStats.m_NumProcessed;
	TEST_ASSERT( actualNumSeen == numSeen );

	size_t actualNumBuilt = nodeStats.m_NumBuilt;
	TEST_ASSERT( actualNumBuilt == numBuilt );
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
void FBuildTest::GetCodeDir( AString & codeDir ) const
{
	// we want the working dir to be the 'Code' directory
	TEST_ASSERT( FileIO::GetCurrentDir( codeDir ) );
    #if defined( __WINDOWS__ )
        const char * codePos = codeDir.FindI( "\\code\\" );
    #else
        const char * codePos = codeDir.FindI( "/code/" );
    #endif
	TEST_ASSERT( codePos );
	codeDir.SetLength( (uint16_t)( codePos - codeDir.Get() + 6 ) );
}

//------------------------------------------------------------------------------
