// FBuild.cpp - The main FBuild interface class
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FBUILD_H
#define FBUILD_FBUILD_H

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/FBuildOptions.h"

#include "Helpers/FBuildStats.h"
#include "WorkerPool/WorkerBrokerage.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/Singleton.h"
#include "Core/Strings/AString.h"
#include "Core/Time/Timer.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Client;
class FileStream;
class ICache;
class JobQueue;
class Node;
class NodeGraph;

// FBuild
//------------------------------------------------------------------------------
class FBuild : public Singleton< FBuild >
{
public:
	explicit FBuild( const FBuildOptions & options = FBuildOptions() );
	~FBuild();

	// initialize the dependency graph, using the BFF config file
	// OR a previously saved NodeGraph DB (if available/matching the BFF)
	bool Initialize( const char * nodeGraphDBFile = nullptr );

	// build a target
	bool Build( const AString & target );
	bool Build( const Array< AString > & targets );
	bool Build( Node * nodeToBuild );

	// after a build we can store progress/parsed rules for next time
	bool SaveDependencyGraph( const char * nodeGraphDBFile = nullptr ) const;

	const FBuildOptions & GetOptions() const { return m_Options; }
	NodeGraph & GetDependencyGraph() const { return *m_DependencyGraph; }
	
	const AString & GetWorkingDir() const { return m_Options.GetWorkingDir(); }

	static const char * GetDependencyGraphFileName();
	static const char * GetDefaultBFFFileName();

	const AString & GetCachePath() const { return m_CachePath; }
	void SetCachePath( const AString & path );

	const AString & GetCachePluginDLL() const { return m_CachePluginDLL; }
	void SetCachePluginDLL( const AString & plugin ) { m_CachePluginDLL = plugin; }

	void GetCacheFileName( uint64_t keyA, uint32_t keyB, uint64_t keyC,
						   AString & path ) const;

	void SetWorkerList( const Array< AString > & workers )		{ m_WorkerList = workers; }
	const Array< AString > & GetWorkerList() const { return m_WorkerList; }

	void SetEnvironmentString( const char * envString, uint32_t size, const AString & libEnvVar );
	inline const char * GetEnvironmentString() const			{ return m_EnvironmentString; }
	inline uint32_t		GetEnvironmentStringSize() const		{ return m_EnvironmentStringSize; }

	void GetLibEnvVar( AString & libEnvVar ) const;

	// stats - read access
	const FBuildStats & GetStats() const	{ return m_BuildStats; }
	// stats - write access
	FBuildStats & GetStatsMutable()			{ return m_BuildStats; }

	// attempt to cleanly stop the build
	static inline void AbortBuild() { s_StopBuild = true; }
	static		  void OnBuildError();
	static inline bool GetStopBuild() { return s_StopBuild; }

	inline ICache * GetCache() const { return m_Cache; }

private:
	void UpdateBuildStatus( const Node * node );

	static bool s_StopBuild;

	NodeGraph * m_DependencyGraph;
	JobQueue * m_JobQueue;
	Client * m_Client; // manage connections to worker servers

	AString m_CachePluginDLL;
	AString m_CachePath;
	ICache * m_Cache;

	Timer m_Timer;
	float m_LastProgressOutputTime;
	float m_LastProgressCalcTime;
	float m_SmoothedProgressCurrent;
	float m_SmoothedProgressTarget;

	FBuildStats m_BuildStats;

	FBuildOptions m_Options;

	WorkerBrokerage m_WorkerBrokerage;

	Array< AString > m_WorkerList;

	AString m_OldWorkingDir;

	// a double-null terminated string
	char *		m_EnvironmentString;
	uint32_t	m_EnvironmentStringSize; // size excluding last null
	AString		m_LibEnvVar; // LIB= value
};

//------------------------------------------------------------------------------
#endif // FBUILD_FBUILD_H
