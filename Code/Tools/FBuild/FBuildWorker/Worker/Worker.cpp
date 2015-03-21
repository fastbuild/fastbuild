// Worker
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#if defined( __WINDOWS__ )
	#include <winsock2.h> // this must be here to avoid windows include order problems
#endif

#include "Worker.h"
#include "WorkerWindow.h"
#include "WorkerSettings.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThreadRemote.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/Network/NetworkStartupHelper.h"
#include "Core/Process/Process.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
Worker::Worker( void * hInstance, const AString & args )
	: m_MainWindow( nullptr )
	, m_ConnectionPool( nullptr )
	, m_NetworkStartupHelper( nullptr )
	, m_BaseArgs( args )
	, m_LastWriteTime( 0 )
	, m_RestartNeeded( false )
{
	m_WorkerSettings = FNEW( WorkerSettings );
	m_NetworkStartupHelper = FNEW( NetworkStartupHelper );
	m_JobQueueRemote = FNEW( JobQueueRemote( Env::GetNumProcessors() ) );
	m_MainWindow = FNEW( WorkerWindow( hInstance ) );
	m_ConnectionPool = FNEW( Server );

	Env::GetExePath( m_BaseExeName );
	if ( m_BaseExeName.Replace( ".copy", "" ) != 1 )
	{
		m_BaseExeName.Clear(); // not running from copy, disable restart detection
	}
	m_BaseArgs.Replace( "-subprocess", "" );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Worker::~Worker()
{
	FDELETE m_NetworkStartupHelper;
	FDELETE m_ConnectionPool;
	FDELETE m_MainWindow;
	FDELETE m_JobQueueRemote;
	FDELETE m_WorkerSettings;

	if ( m_RestartNeeded )
	{
		Process p;
		p.Spawn( m_BaseExeName.Get(), 
			     m_BaseArgs.Get(), 
				 nullptr,	// default workingDir
				 nullptr ); // default env
		p.Detach();
	}
}

// Work
//------------------------------------------------------------------------------
int Worker::Work()
{
	// start listening
	if ( m_ConnectionPool->Listen( Protocol::PROTOCOL_PORT ) == false )
	{
		ShowMessageBox( "Failed to listen on port %u.  Check port is not in use.", Protocol::PROTOCOL_PORT );
		return -1;
	}

	// Special folder for Orbis Clang
	// We just create this folder whether it's needed or not
	{
		AStackString<> tmpPath;
		VERIFY( FileIO::GetTempDir( tmpPath ) );
        #if defined( __WINDOWS__ )
            tmpPath += ".fbuild.tmp\\target\\include";
        #else
            tmpPath += "_fbuild.tmp/target/include";
        #endif
		if ( !FileIO::EnsurePathExists( tmpPath ) )
		{
			ShowMessageBox( "Failed to initialize tmp folder.  Error: 0x%x", Env::GetLastErr() );
			return -2;
		}
        #if defined( __WINDOWS__ )
            tmpPath += "\\.lock";
        #else
            tmpPath += "/_lock";
        #endif
		if ( !m_TargetIncludeFolderLock.Open( tmpPath.Get(), FileStream::WRITE_ONLY ) )
		{
			ShowMessageBox( "Failed to lock tmp folder.  Error: 0x%x", Env::GetLastErr() );
			return -2;
		}
	}

	while ( WorkerWindow::Get().WantToQuit() == false )
	{
		UpdateAvailability();

		UpdateUI();

		CheckForExeUpdate();

		PROFILE_SYNCHRONIZE

		Thread::Sleep( 100 );
	}

	// allow to UI to shutdown
	// the application MUST NOT try to update the UI from this point on
	m_MainWindow->SetAllowQuit();

	m_WorkerBrokerage.SetAvailability( false );

	return 0;
}

// UpdateAvailability
//------------------------------------------------------------------------------
void Worker::UpdateAvailability()
{
	m_IdleDetection.Update();

	WorkerSettings & ws = WorkerSettings::Get();
	uint32_t numCPUsToUse = ws.GetNumCPUsToUse();
	switch( ws.GetMode() )
	{
		case WorkerSettings::WHEN_IDLE:
		{
			if ( m_IdleDetection.IsIdle() == false )
			{
				numCPUsToUse = 0;
			}
			break;
		}
		case WorkerSettings::DEDICATED:
		{
			break; // use all allocated cpus
		}
		case WorkerSettings::DISABLED:
		{
			numCPUsToUse = 0;
			break;
		}
	}

	// don't accept any new work while waiting for a restart
	if ( m_RestartNeeded )
	{
		numCPUsToUse = 0;
	}

	WorkerThreadRemote::SetNumCPUsToUse( numCPUsToUse );

	m_WorkerBrokerage.SetAvailability( numCPUsToUse > 0 );
}

// UpdateUI
//------------------------------------------------------------------------------
void Worker::UpdateUI()
{
	// throttle UI updates
	if ( m_UIUpdateTimer.GetElapsed() < 0.25f )
	{
		return;
	}

	// title bar
	size_t numConnections = m_ConnectionPool->GetNumConnections();
	AStackString<> status;
	status.Format( "%u Connections", (uint32_t)numConnections );
	if ( m_RestartNeeded )
	{
		status += " (Restart Pending)";
	}
	m_MainWindow->SetStatus( status.Get() );

	// thread output
	JobQueueRemote & jqr = JobQueueRemote::Get();
	const size_t numWorkers = jqr.GetNumWorkers();
	for ( size_t i=0; i<numWorkers; ++i )
	{
		// get status of worker
		AStackString<> workerStatus;
		AStackString<> hostName;
		bool isIdle;
		jqr.GetWorkerStatus( i, hostName, workerStatus, isIdle );

		// are we syncing tools?
		if ( isIdle )
		{
			AStackString<> statusStr;
			if ( m_ConnectionPool->IsSynchingTool( statusStr ) )
			{
				// show status of synchronization
				workerStatus = statusStr;
			}
		}

		// reflect in UI
		m_MainWindow->SetWorkerState( i, hostName, workerStatus );
	}

	m_UIUpdateTimer.Start();
}

// CheckForExeUpdate
//------------------------------------------------------------------------------
void Worker::CheckForExeUpdate()
{
	// if a restart is pending, can we restart yet?
	if ( m_RestartNeeded )
	{
		// can we restart yet?
		if ( JobQueueRemote::Get().HaveWorkersStopped() )
		{
			WorkerWindow::Get().SetWantToQuit();
		}

		return;
	}

	if ( m_BaseExeName.IsEmpty() )
	{
		return; // not running as a copy to allow restarts
	}

	/// get the current last write time
	uint64_t lastWriteTime = FileIO::GetFileLastWriteTime( m_BaseExeName );

	// store the time when we were launch (first time in this function)
	if ( m_LastWriteTime == 0 )
	{		
		m_LastWriteTime = lastWriteTime;
		return;
	}

	// has exe changed?
	if ( lastWriteTime != m_LastWriteTime )
	{
		m_RestartNeeded = true;
		JobQueueRemote::Get().SignalStopWorkers();
	}
}

// ShowMessageBox
//------------------------------------------------------------------------------
/*static*/ void Worker::ShowMessageBox( const char * fmtString, ... )
{
	AStackString<> buffer;

	va_list args;
	va_start(args, fmtString);
	buffer.VFormat( fmtString, args );
	va_end( args );

	#if defined( __WINDOWS__ )
		::MessageBox( nullptr, buffer.Get(), "FBuild Worker", MB_OK );
	#elif defined( __APPLE__ )
		// TODO:MAC Implement ShowMessageBox
	#elif defined( __LINUX__ )
		// TODO:LINUX Implement ShowMessageBox
	#else
		#error Unknown Platform
	#endif
}

//------------------------------------------------------------------------------
