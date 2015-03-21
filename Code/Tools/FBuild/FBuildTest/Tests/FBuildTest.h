// FBuiltTest.h
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_TEST_H
#define FBUILD_TEST_H

// Includes
//------------------------------------------------------------------------------
#include "TestFramework/UnitTest.h"

#include "Tools/FBuild/FBuildCore/Graph/Node.h"

// Forward Declarations
//------------------------------------------------------------------------------
struct FBuildStats;

// FBuildTest
//------------------------------------------------------------------------------
class FBuildTest : public UnitTest
{
protected:
	// helpers to clear and check for generated files
	void EnsureFileDoesNotExist( const char * fileName ) const;
	void EnsureFileDoesNotExist( const AString & fileName ) const { EnsureFileDoesNotExist( fileName.Get() ); }
	void EnsureFileExists( const char * fileName ) const;
	void EnsureFileExists( const AString & fileName ) const { EnsureFileExists( fileName.Get() ); }

	// Helpers to check build results
	void CheckStatsNode( const FBuildStats & stats, size_t numSeen, size_t numBuilt, Node::Type nodeType ) const;
	void CheckStatsTotal( const FBuildStats & stats, size_t numSeen, size_t numBuilt ) const;
	void CheckStatsNode( size_t numSeen, size_t numBuilt, Node::Type nodeType ) const;
	void CheckStatsTotal( size_t numSeen, size_t numBuilt ) const;

	// other helpers
	void GetCodeDir( AString & codeDir ) const;
};

//------------------------------------------------------------------------------
#endif // FBUILD_TEST_H
