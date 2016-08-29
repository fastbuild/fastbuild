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
	virtual void PreTest() const override;
	virtual void PostTest() const override;

	// helpers to clear and check for generated files
	void EnsureFileDoesNotExist( const char * fileName ) const;
	void EnsureFileDoesNotExist( const AString & fileName ) const { EnsureFileDoesNotExist( fileName.Get() ); }
	void EnsureFileExists( const char * fileName ) const;
	void EnsureFileExists( const AString & fileName ) const { EnsureFileExists( fileName.Get() ); }
	void EnsureDirDoesNotExist( const char * dirPath ) const;
	void EnsureDirDoesNotExist( const AString & dirPath ) const { EnsureDirDoesNotExist( dirPath.Get() ); }
	void EnsureDirExists( const char * dirPath ) const;
	void EnsureDirExists( const AString & dirPath ) const { EnsureDirExists( dirPath.Get() ); }

	// Helpers to check build results
	void CheckStatsNode( const FBuildStats & stats, size_t numSeen, size_t numBuilt, Node::Type nodeType ) const;
	void CheckStatsTotal( const FBuildStats & stats, size_t numSeen, size_t numBuilt ) const;
	void CheckStatsNode( size_t numSeen, size_t numBuilt, Node::Type nodeType ) const;
	void CheckStatsTotal( size_t numSeen, size_t numBuilt ) const;

	// other helpers
	void GetCodeDir( AString & codeDir ) const;

	const AString & GetRecordedOutput() const { return s_RecordedOutput; }
private:
	static bool LoggingCallback( const char * message );
	static AString s_RecordedOutput;
};

//------------------------------------------------------------------------------
#endif // FBUILD_TEST_H
