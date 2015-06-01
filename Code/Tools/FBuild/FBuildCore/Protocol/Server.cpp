// Server.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Server.h"
#include "Protocol.h"

#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Helpers/ToolManifest.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerThreadRemote.h"

#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Strings/AStackString.h"

// Defines
//------------------------------------------------------------------------------
#define SERVER_STATUS_SEND_FREQUENCY ( 1.0f )

// CONSTRUCTOR
//------------------------------------------------------------------------------
Server::Server()
	: m_ShouldExit( false )
	, m_Exited( false )
	, m_RequestJobs( true )
	, m_ClientList( 32, true )
{
	m_Thread = Thread::CreateThread( ThreadFuncStatic,
									 "Server",
									 ( 64 * KILOBYTE ),
									 this );
	ASSERT( m_Thread );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Server::~Server()
{
	m_ShouldExit = true;
	while ( m_Exited == false )
	{
		Thread::Sleep( 1 );
	}

	ShutdownAllConnections();

	Thread::CloseHandle( m_Thread );

	const ToolManifest * const * end = m_Tools.End();
	for ( ToolManifest ** it = m_Tools.Begin(); it != end; ++it )
	{
		FDELETE *it;
	}
}

// GetHostForJob
//------------------------------------------------------------------------------
/*static*/ void Server::GetHostForJob( const Job * job, AString & hostName )
{
	const ClientState * cs = (const ClientState *)job->GetUserData();
	if ( cs )
	{
		hostName = cs->m_HostName;
	}
	else
	{
		hostName.Clear();
	}
}

// IsSynchingTool
//------------------------------------------------------------------------------
bool Server::IsSynchingTool( AString & statusStr ) const
{
	MutexHolder manifestMH( m_ToolManifestsMutex ); // ensure we don't make redundant requests

	const ToolManifest * const * end = m_Tools.End();
	ToolManifest ** it = m_Tools.Begin();
	while ( it != end )
	{
		if ( ( *it )->IsSynchronized() == false )
		{
			uint32_t synchDone;
			uint32_t synchTotal;
			bool synching = ( *it )->GetSynchronizationStatus( synchDone, synchTotal );
			if ( synching )
			{
				statusStr.Format( "Synchronizing Compiler %2.1f / %2.1f MiB\n", 
									(float)synchDone / (float)MEGABYTE,
									(float)synchTotal / (float)MEGABYTE );
				return true;
			}
		}
		++it;
	}

	return false; // no toolchain is currently synching
}

// OnConnected
//------------------------------------------------------------------------------
/*virtual*/ void Server::OnConnected( const ConnectionInfo * connection )
{
	ClientState * cs = FNEW( ClientState( connection ) );
	connection->SetUserData( cs );

	MutexHolder mh( m_ClientListMutex );
	m_ClientList.Append( cs );
}

//------------------------------------------------------------------------------
/*virtual*/ void Server::OnDisconnected( const ConnectionInfo * connection )
{
	ASSERT( connection );
	ClientState * cs = (ClientState *)connection->GetUserData();
	ASSERT( cs );

	// Unhook any jobs which are queued or in progress for this client
	// - deletes the queued jobs
	// - unhooks the UserData for in-progress jobs so the result is discarded on completion
	JobQueueRemote & jqr = JobQueueRemote::Get();
	jqr.CancelJobsWithUserData( cs );

	// check if any tool chain was being sync'd from this Client
	Array< ToolManifest * > cancelledManifests( 0, true );
	{
		MutexHolder manifestMH( m_ToolManifestsMutex );
		const ToolManifest * const * end = m_Tools.End();
		ToolManifest ** it = m_Tools.Begin();
		while ( it != end )
		{
			// if synchronizing from connection that was just disconnected...
			ToolManifest * tm = *it;
			if ( ( tm->IsSynchronized() == false ) &&
				 ( tm->GetUserData() == connection ) )
			{
				// ...flag any expected files as not synching
				tm->CancelSynchronizingFiles();
				tm->SetUserData( nullptr );
				cancelledManifests.Append( tm );
			}
			++it;
		}
	}

	// free the serverstate structure
	MutexHolder mh( m_ClientListMutex );
	ClientState ** iter = m_ClientList.Find( cs );
	ASSERT( iter );
	m_ClientList.Erase( iter );

	// because we cancelled manifest syncrhonization, we need to check if other
	// connections are waiting for the same manifest
	{
		ClientState ** it = m_ClientList.Begin();
		const ClientState * const * end = m_ClientList.End();
		for ( ; it != end; ++it )
		{
			ClientState * otherCS = *it;

			MutexHolder mh2( otherCS->m_Mutex );
			const Job * const * jEnd = otherCS->m_WaitingJobs.End();
			for ( Job ** jIt = otherCS->m_WaitingJobs.Begin(); jIt != jEnd; ++jIt )
			{
				Job * j = *jIt;
				ToolManifest * jMan = j->GetToolManifest();
				if ( cancelledManifests.Find( jMan ) )
				{
					RequestMissingFiles( otherCS->m_Connection, jMan );
				}
			}
		}
	}

	// This is usually null here, but might need to be freed if
	// we had the connection drop between message and payload
	FREE( (void *)( cs->m_CurrentMessage ) );

	// delete any jobs where we were waiting on Tool synchronization
	const Job * const * end = cs->m_WaitingJobs.End();
	for ( Job ** it=cs->m_WaitingJobs.Begin(); it!=end; ++it )
	{
		delete *it;
	}

	FDELETE cs;
}

// OnReceive
//------------------------------------------------------------------------------
/*virtual*/ void Server::OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory )
{
	keepMemory = true; // we'll take care of freeing the memory

	ClientState * cs = (ClientState *)connection->GetUserData();
	ASSERT( cs );

	// are we expecting a msg, or the payload for a msg?
	void * payload = nullptr;
	size_t payloadSize = 0;
	if ( cs->m_CurrentMessage == nullptr )
	{
		// message
		cs->m_CurrentMessage = reinterpret_cast< const Protocol::IMessage * >( data );
		if ( cs->m_CurrentMessage->HasPayload() )
		{
			return;
		}
	}
	else
	{
		// payload
		ASSERT( cs->m_CurrentMessage->HasPayload() );
		payload = data;
		payloadSize = size;
	}


	// determine message type
	const Protocol::IMessage * imsg = cs->m_CurrentMessage;
	Protocol::MessageType messageType = imsg->GetType();

	PROTOCOL_DEBUG( "Client -> Server : %u (%s)\n", messageType, GetProtocolMessageDebugName( messageType ) );

	switch ( messageType )
	{
		case Protocol::MSG_CONNECTION:
		{
			const Protocol::MsgConnection * msg = static_cast< const Protocol::MsgConnection * >( imsg );
			Process( connection, msg ); 
			break;
		}
		case Protocol::MSG_STATUS:
		{
			const Protocol::MsgStatus * msg = static_cast< const Protocol::MsgStatus * >( imsg );
			Process( connection, msg );
			break;
		}
		case Protocol::MSG_NO_JOB_AVAILABLE:
		{
			const Protocol::MsgNoJobAvailable * msg = static_cast< const Protocol::MsgNoJobAvailable * >( imsg );
			Process( connection, msg );
			break;
		}
		case Protocol::MSG_JOB:
		{
			const Protocol::MsgJob * msg = static_cast< const Protocol::MsgJob * >( imsg );
			Process( connection, msg, payload, payloadSize );
			break;
		}
		case Protocol::MSG_MANIFEST:
		{
			const Protocol::MsgManifest * msg = static_cast< const Protocol::MsgManifest * >( imsg );
			Process( connection, msg, payload, payloadSize );
			break;
		}
		case Protocol::MSG_FILE:
		{
			const Protocol::MsgFile * msg = static_cast< const Protocol::MsgFile * >( imsg );
			Process( connection, msg, payload, payloadSize );
			break;
		}
		default:
		{
			// unknown message type
			ASSERT( false ); // this indicates a protocol bug
			Disconnect( connection );
			break;
		}
	}

	// free everything
	FREE( (void *)( cs->m_CurrentMessage ) );
	FREE( payload );
	cs->m_CurrentMessage = nullptr;
}

// Process( MsgConnection )
//------------------------------------------------------------------------------
void Server::Process( const ConnectionInfo * connection, const Protocol::MsgConnection * msg )
{
	// check for valid/supported protocol version
	if ( msg->GetProtocolVersion() != Protocol::PROTOCOL_VERSION )
	{
		AStackString<> remoteAddr;
		TCPConnectionPool::GetAddressAsString( connection->GetRemoteAddress(), remoteAddr );
		FLOG_WARN( "Disconnecting '%s' due to bad protocol version\n", remoteAddr.Get() );
		Disconnect( connection );
		return;
	}

	// take note of initial status of client
	ClientState * cs = (ClientState *)connection->GetUserData();
	cs->m_NumJobsAvailable = msg->GetNumJobsAvailable();
	cs->m_HostName = msg->GetHostName();
}

// Process( MsgStatus )
//------------------------------------------------------------------------------
void Server::Process( const ConnectionInfo * connection, const Protocol::MsgStatus * msg )
{
	// take note of latest status of client
	ClientState * cs = (ClientState *)connection->GetUserData();
	cs->m_NumJobsAvailable = msg->GetNumJobsAvailable();
}

// Process( MsgNoJobAvailable )
//------------------------------------------------------------------------------
void Server::Process( const ConnectionInfo * connection, const Protocol::MsgNoJobAvailable * )
{
	// We requested a job, but the client didn't have any left
	ClientState * cs = (ClientState *)connection->GetUserData();
	MutexHolder mh( cs->m_Mutex );
	ASSERT( cs->m_NumJobsRequested > 0 );
	cs->m_NumJobsRequested--;
}

// Process( MsgJob )
//------------------------------------------------------------------------------
void Server::Process( const ConnectionInfo * connection, const Protocol::MsgJob * msg, const void * payload, size_t payloadSize )
{
	ClientState * cs = (ClientState *)connection->GetUserData();
	MutexHolder mh( cs->m_Mutex );
	ASSERT( cs->m_NumJobsRequested > 0 );
	cs->m_NumJobsRequested--;
	cs->m_NumJobsActive++;

	// deserialize job
	ConstMemoryStream ms( payload, payloadSize );

	Job * job = FNEW( Job( ms ) );
	job->SetUserData( cs );

	// 
	const uint64_t toolId = msg->GetToolId();
	ASSERT( toolId );

	MutexHolder manifestMH( m_ToolManifestsMutex ); // ensure we don't make redundant requests

	ToolManifest ** found = m_Tools.FindDeref( toolId );
	ToolManifest * manifest = found ? *found : nullptr;
	if ( manifest )
	{
		job->SetToolManifest( manifest );
		if ( manifest->IsSynchronized() )
		{
			// we have all the files - we can do the job
			JobQueueRemote::Get().QueueJob( job );
			return;		
		}

		// missing some files - request them		
		RequestMissingFiles( connection, manifest );
	}
	else
	{
		// first time seeing this tool

		// create manifest object
		manifest = FNEW( ToolManifest( toolId ) );
		job->SetToolManifest( manifest );
		m_Tools.Append( manifest );

		// request manifest of tool chain
		Protocol::MsgRequestManifest reqMsg( toolId );
		reqMsg.Send( connection );
	}

	// can't start job yet - put it on hold
	cs->m_WaitingJobs.Append( job );
}

// Process( MsgManifest )
//------------------------------------------------------------------------------
void Server::Process( const ConnectionInfo * connection, const Protocol::MsgManifest * msg, const void * payload, size_t payloadSize )
{
	ToolManifest * manifest = nullptr;
	const uint64_t toolId = msg->GetToolId();
	ConstMemoryStream ms( payload, payloadSize );

	{
		MutexHolder manifestMH( m_ToolManifestsMutex ); // ensure we don't make redundant requests

		// fill out the received manifest
		ToolManifest ** found = m_Tools.FindDeref( toolId );
		ASSERT( found );
		manifest = *found;
		manifest->Deserialize( ms, true ); // true == remote
	}

	// manifest has checked local files, from previous sessions an may
	// be synchronized
	if ( manifest->IsSynchronized() )
	{
		CheckWaitingJobs( manifest );
		return;
	}

	RequestMissingFiles( connection, manifest );
}

// Process( MsgFile )
//------------------------------------------------------------------------------
void Server::Process( const ConnectionInfo * connection, const Protocol::MsgFile * msg, const void * payload, size_t payloadSize )
{
	const uint64_t toolId = msg->GetToolId();
	const uint32_t fileId = msg->GetFileId();

	// Update the Manifest
	ToolManifest * manifest = nullptr;
	{
		MutexHolder manifestMH( m_ToolManifestsMutex );

		// fill out the received manifest
		ToolManifest ** found = m_Tools.FindDeref( toolId );
		ASSERT( found );
		manifest = *found;
		ASSERT( manifest->GetUserData() == connection ); (void)connection;

		if ( manifest->ReceiveFileData( fileId, payload, payloadSize ) == false )
		{
			// something went wrong storing the file
			FLOG_WARN( "Failed to store fileId %u for manifest 0x%llx\n", fileId, toolId );
			Disconnect( connection );
			return;
		}

		if ( manifest->IsSynchronized() == false )
		{
			// wait for more files
			return;
		}
		manifest->SetUserData( nullptr );
	}

	// ToolChain is now synchronized
	// Allow any jobs that were waiting on it to start
	CheckWaitingJobs( manifest );
}

// CheckWaitingJobs
//------------------------------------------------------------------------------
void Server::CheckWaitingJobs( const ToolManifest * manifest )
{
	// queue for start any jobs that may now be ready
    #ifdef ASSERTS_ENABLED
        bool atLeastOneJobStarted = false;
    #endif

	MutexHolder mhC( m_ClientListMutex );
	const ClientState * const * end = m_ClientList.End();
	for ( ClientState ** it = m_ClientList.Begin(); it!=end; ++it )
	{
		// For each connected client...
		ClientState * cs = *it;
		MutexHolder mh2( cs->m_Mutex );

		// .. check all jobs waiting for ToolManifests
		int32_t numJobs = (int32_t)cs->m_WaitingJobs.GetSize();
		for ( int32_t i=( numJobs -1 ); i >= 0; --i )
		{
			Job * job = cs->m_WaitingJobs[ i ];
			ToolManifest * manifestForThisJob = job->GetToolManifest();
			ASSERT( manifestForThisJob );
			if ( manifestForThisJob == manifest )
			{
				cs->m_WaitingJobs.EraseIndex( i );
				JobQueueRemote::Get().QueueJob( job );
				PROTOCOL_DEBUG( "Server: Job %x can now be started\n", job );
                #ifdef ASSERTS_ENABLED
                    atLeastOneJobStarted = true;
                #endif
			}
		}
	}

	// We should only have called this function when a ToolChain sync was complete
	// so at least 1 job should have been waiting for it
	ASSERT( atLeastOneJobStarted );
}


// ThreadFuncStatic
//------------------------------------------------------------------------------
/*static*/ uint32_t Server::ThreadFuncStatic( void * param )
{
	Server * s = (Server *)param;
	s->ThreadFunc();
	return 0;
}

// ThreadFunc
//------------------------------------------------------------------------------
void Server::ThreadFunc()
{
	while ( m_ShouldExit == false )
	{
		FindNeedyClients();

		FinalizeCompletedJobs();

		SendServerStatus();

		Thread::Sleep( 16 );
	}

	m_Exited = true;
}

// FindNeedyClients
//------------------------------------------------------------------------------
void Server::FindNeedyClients()
{
	if ( m_RequestJobs == false )
	{
		return;
	}

	MutexHolder mh( m_ClientListMutex );

	// determine job availability
	int availableJobs = (int)WorkerThreadRemote::GetNumCPUsToUse();

	ClientState ** iter = m_ClientList.Begin();
	const ClientState * const * end = m_ClientList.End();
	for ( ; iter != end; ++iter )
	{
		ClientState * cs = *iter;

		MutexHolder mh2( cs->m_Mutex );

		// any jobs requested or in progress reduce the available count
		int reservedJobs = cs->m_NumJobsRequested +
							  cs->m_NumJobsActive;
		availableJobs -= reservedJobs;
		if ( availableJobs <= 0 )
		{
			return;
		}
	}

	// we have some jobs available

	// sort clients to find neediest first
	m_ClientList.SortDeref();

	iter = m_ClientList.Begin();
	for ( ; iter != end; ++iter )
	{
		ClientState * cs = *iter;

		MutexHolder mh2( cs->m_Mutex );

		size_t reservedJobs = cs->m_NumJobsRequested +
							  cs->m_NumJobsActive;
		if ( reservedJobs >= cs->m_NumJobsAvailable )
		{
			continue; // we've maxed out the requests to this worker
		}

		// request job from this client
		Protocol::MsgRequestJob msg;
		msg.Send( cs->m_Connection );
		cs->m_NumJobsRequested++;
		availableJobs--;
		if ( availableJobs == 0 )
		{
			return; // used up all jobs
		}
	}
}

// FinalizeCompletedJobs
//------------------------------------------------------------------------------
void Server::FinalizeCompletedJobs()
{
	JobQueueRemote & jcr = JobQueueRemote::Get();
	while ( Job * job = jcr.GetCompletedJob() )
	{
		// get associated connection
		ClientState * cs = (ClientState *)job->GetUserData();

		MutexHolder mh( m_ClientListMutex );

		bool connectionStillActive = ( m_ClientList.Find( cs ) != nullptr );
		if ( connectionStillActive )
		{
			Node::State result = job->GetNode()->GetState();
			ASSERT( ( result == Node::UP_TO_DATE ) || ( result == Node::FAILED ) );

			MemoryStream ms;
			ms.Write( job->GetJobId() );
			ms.Write( job->GetNode()->GetName() );
			ms.Write( result == Node::UP_TO_DATE );
			ms.Write( job->GetSystemErrorCount() > 0 );
			ms.Write( job->GetMessages() );
			ms.Write( job->GetNode()->GetLastBuildTime() );

			// write the data - build result for success, or output+errors for failure
			ms.Write( (uint32_t)job->GetDataSize() );
			ms.WriteBuffer( job->GetData(), job->GetDataSize() );

			MutexHolder mh2( cs->m_Mutex );
			ASSERT( cs->m_NumJobsActive );
			cs->m_NumJobsActive--;

			Protocol::MsgJobResult msg;
			msg.Send( cs->m_Connection, ms );
		}
		else
		{
			// we might get here without finding the connection
			// (if the connection was lost before we completed)
		}

		FDELETE job;
	}
}

// SendServerStatus
//------------------------------------------------------------------------------
void Server::SendServerStatus()
{
	MutexHolder mh( m_ClientListMutex );

	const ClientState * const * end = m_ClientList.End();
	for ( ClientState ** it = m_ClientList.Begin(); it !=  end; ++it )
	{
		ClientState * cs = *it;
		MutexHolder mh2( cs->m_Mutex );
		if ( cs->m_StatusTimer.GetElapsedMS() < Protocol::SERVER_STATUS_FREQUENCY_MS )
		{
			continue;
		}
		cs->m_StatusTimer.Start();

		Protocol::MsgServerStatus msg;
		msg.Send( cs->m_Connection );
	}
}

// RequestMissingFiles
//------------------------------------------------------------------------------
void Server::RequestMissingFiles( const ConnectionInfo * connection, ToolManifest * manifest ) const
{
	MutexHolder manifestMH( m_ToolManifestsMutex );

	const Array< ToolManifest::File > & files = manifest->GetFiles();
	const size_t numFiles = files.GetSize();
	for ( size_t i=0; i<numFiles; ++i )
	{
		const ToolManifest::File & f = files[ i ];
		if ( f.m_SyncState == ToolManifest::File::NOT_SYNCHRONIZED )
		{
			// request this file
			Protocol::MsgRequestFile reqFileMsg( manifest->GetToolId(), (uint32_t)i );
			reqFileMsg.Send( connection );

			// prevent it being requested again
			manifest->MarkFileAsSynchronizing( i );

			// either this is the first file being synchronized, or we
			// are synchronizing multiple files from the same connection
			// (it should not be possible to have files requested from different connections)
			ASSERT( ( manifest->GetUserData() == nullptr ) || ( manifest->GetUserData() == connection ) );
			manifest->SetUserData( (void *)connection );
		}
	}
}

//------------------------------------------------------------------------------
