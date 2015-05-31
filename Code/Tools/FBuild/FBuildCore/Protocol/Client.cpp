// Client.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Client.h"

#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/CompilerNode.h"
#include "Tools/FBuild/FBuildCore/Graph/FileNode.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Graph/ObjectNode.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/Job.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"

#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Math/Random.h"
#include "Core/Profile/Profile.h"

// Defines
//------------------------------------------------------------------------------
#define CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS ( 1.0f )
#define CONNECTION_LIMIT ( 15 )
#define CONNECTION_REATTEMPT_DELAY_TIME ( 10.0f )
#define SYSTEM_ERROR_ATTEMPT_COUNT ( 3 )

// CONSTRUCTOR
//------------------------------------------------------------------------------
Client::Client( const Array< AString > & workerList )
	: m_WorkerList( workerList )
	, m_ShouldExit( false )
	, m_Exited( false )
{
	// allocate space for server states
	m_ServerList.SetSize( workerList.GetSize() );

	m_Thread = Thread::CreateThread( ThreadFuncStatic,
									 "Client",
									 ( 64 * KILOBYTE ),
									 this );
	ASSERT( m_Thread );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Client::~Client()
{
	SetShuttingDown();

	m_ShouldExit = true;
	while ( m_Exited == false )
	{
		Thread::Sleep( 1 );
	}

	ShutdownAllConnections();

	Thread::CloseHandle( m_Thread );
}

//------------------------------------------------------------------------------
/*virtual*/ void Client::OnDisconnected( const ConnectionInfo * connection )
{
	ASSERT( connection );
	ServerState * ss = (ServerState *)connection->GetUserData();
	ASSERT( ss );

	MutexHolder mh( m_ServerListMutex );
	if ( ss->m_Jobs.IsEmpty() == false )
	{
		Job ** it = ss->m_Jobs.Begin();
		const Job * const * end = ss->m_Jobs.End();
		while ( it != end )
		{
			JobQueue::Get().ReturnUnfinishedDistributableJob( *it );
			++it;
		}
		ss->m_Jobs.Clear();
	}

	// This is usually null here, but might need to be freed if
	// we had the connection drop between message and payload
	FREE( (void *)( ss->m_CurrentMessage ) );

	ss->m_Connection = nullptr;
	ss->m_CurrentMessage = nullptr;
}

// ThreadFuncStatic
//------------------------------------------------------------------------------
/*static*/ uint32_t Client::ThreadFuncStatic( void * param )
{
	Client * c = (Client *)param;
	c->ThreadFunc();
	return 0;
}

// ThreadFunc
//------------------------------------------------------------------------------
void Client::ThreadFunc()
{
    PROFILE_FUNCTION

	// ensure first status update will be sent more rapidly
	m_StatusUpdateTimer.Start( CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS * 0.5f );

	for ( ;; )
	{
		LookForWorkers();
		if ( m_ShouldExit )
		{
			break;
		}

		CommunicateJobAvailability();
		if ( m_ShouldExit )
		{
			break;
		}

		CheckForTimeouts();
		if ( m_ShouldExit )
		{
			break;
		}

		Thread::Sleep( 1 );
	}

	m_Exited = true;
}

// LookForWorkers
//------------------------------------------------------------------------------
void Client::LookForWorkers()
{
	MutexHolder mh( m_ServerListMutex );

	const size_t numWorkers( m_ServerList.GetSize() );

	// find out how many connections we have now
	size_t numConnections = 0;
	for ( size_t i=0; i<numWorkers; i++ )
	{
		if ( m_ServerList[ i ].m_Connection )
		{
			numConnections++;
		}
	}

	// limit maximum concurrent connections
	if ( numConnections >= CONNECTION_LIMIT )
	{
		return;
	}

	// if we're connected to every possible worker already
	if ( numConnections == numWorkers )
	{
		return;
	}

	// randomize the start index to better distribute workers when there 
	// are many workers/clients - otherwise all clients will attempt to connect 
	// to the first CONNECTION_LIMIT workers
	Random r;
	size_t startIndex = r.GetRandIndex( (uint32_t)numWorkers );

	// find someone to connect to
	for ( size_t j=0; j<numWorkers; j++ )
	{
		const size_t i( ( j + startIndex ) % numWorkers );

		ServerState & ss = m_ServerList[ i ];
		if ( ss.m_Connection )
		{
			continue;
		}

		// ignore blacklisted workers
		if ( ss.m_Blacklisted )
		{
			continue;
		}

		// lock the server state
		MutexHolder mhSS( ss.m_Mutex );

		ASSERT( ss.m_Jobs.IsEmpty() );

		if ( ss.m_DelayTimer.GetElapsed() < CONNECTION_REATTEMPT_DELAY_TIME )
		{
			continue;
		}

		const ConnectionInfo * ci = Connect( m_WorkerList[ i ], Protocol::PROTOCOL_PORT );
		if ( ci == nullptr )
		{
			ss.m_DelayTimer.Start(); // reset connection attempt delay
		}
		else
		{
			const uint32_t numJobsAvailable( JobQueue::IsValid() ? (uint32_t)JobQueue::Get().GetNumDistributableJobsAvailable() : 0 );

			ci->SetUserData( &ss );
			ss.m_Connection = ci; // success!
			ss.m_NumJobsAvailable = numJobsAvailable;
			ss.m_StatusTimer.Start();

			// send connection msg
			Protocol::MsgConnection msg( numJobsAvailable );
			MutexHolder mh2( ss.m_Mutex );
			msg.Send( ci );
		}

		// limit to one connection attempt per iteration
		return;
	}
}

// CommunicateJobAvailability
//------------------------------------------------------------------------------
void Client::CommunicateJobAvailability()
{
	// too soon since last status update?
	if ( m_StatusUpdateTimer.GetElapsed() < CLIENT_STATUS_UPDATE_FREQUENCY_SECONDS )
	{
		return;
	}

	m_StatusUpdateTimer.Start(); // reset time

	// possible for job queue to not exist yet
	if ( !JobQueue::IsValid() )
	{
		return;
	}

	// has status changed since we last sent it?
	uint32_t numJobsAvailable = (uint32_t)JobQueue::Get().GetNumDistributableJobsAvailable();
	Protocol::MsgStatus msg( numJobsAvailable );

	MutexHolder mh( m_ServerListMutex );
	if ( m_ServerList.IsEmpty() )
	{
		return; // no servers to communicate with
	}

	// update each server to know how many jobs we have now
	ServerState * it = m_ServerList.Begin();
	const ServerState * const end = m_ServerList.End();
	while ( it != end )
	{
		if ( it->m_Connection )
		{
			MutexHolder ssMH( it->m_Mutex );
			if ( it->m_NumJobsAvailable != numJobsAvailable )
			{
                PROFILE_SECTION( "UpdateJobAvailability" )
				msg.Send( it->m_Connection );
				it->m_NumJobsAvailable = numJobsAvailable;
			}
		}
		++it;
	}
}

// CheckForTimeouts
//------------------------------------------------------------------------------
void Client::CheckForTimeouts()
{
	MutexHolder mh( m_ServerListMutex );

	// update each server to know how many jobs we have now
	const ServerState * const end = m_ServerList.End();
	for ( ServerState * it = m_ServerList.Begin(); it != end; ++it )
	{
		ServerState & ss = *it;
		if ( ss.m_Connection )
		{
			MutexHolder ssMH( it->m_Mutex );
			if ( ss.m_StatusTimer.GetElapsedMS() >= Protocol::SERVER_STATUS_TIMEOUT )
			{
				Disconnect( ss.m_Connection );
			}
		}
		else
		{
			ASSERT( ss.m_Jobs.IsEmpty() );
		}
	}
}

// OnReceive
//------------------------------------------------------------------------------
/*virtual*/ void Client::OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory )
{
	keepMemory = true; // we'll take care of freeing the memory

	MutexHolder mh( m_ServerListMutex );

	ServerState * ss = (ServerState *)connection->GetUserData();
	ASSERT( ss );

	// are we expecting a msg, or the payload for a msg?
	void * payload = nullptr;
	size_t payloadSize = 0;
	if ( ss->m_CurrentMessage == nullptr )
	{
		// message
		ss->m_CurrentMessage = reinterpret_cast< const Protocol::IMessage * >( data );
		if ( ss->m_CurrentMessage->HasPayload() )
		{
			return;
		}
	}
	else
	{
		// payload
		ASSERT( ss->m_CurrentMessage->HasPayload() );
		payload = data;
		payloadSize = size;
	}

	// determine message type
	const Protocol::IMessage * imsg = ss->m_CurrentMessage;
	Protocol::MessageType messageType = imsg->GetType();

	PROTOCOL_DEBUG( "Server -> Client : %u (%s)\n", messageType, GetProtocolMessageDebugName( messageType ) );

	switch ( messageType )
	{
		case Protocol::MSG_REQUEST_JOB:
		{
			const Protocol::MsgRequestJob * msg = static_cast< const Protocol::MsgRequestJob * >( imsg );
			Process( connection, msg ); 
			break;
		}
		case Protocol::MSG_JOB_RESULT:
		{
			const Protocol::MsgJobResult * msg = static_cast< const Protocol::MsgJobResult * >( imsg );
			Process( connection, msg, payload, payloadSize ); 
			break;
		}
		case Protocol::MSG_REQUEST_MANIFEST:
		{
			const Protocol::MsgRequestManifest * msg = static_cast< const Protocol::MsgRequestManifest * >( imsg );
			Process( connection, msg ); 
			break;
		}
		case Protocol::MSG_REQUEST_FILE:
		{
			const Protocol::MsgRequestFile * msg = static_cast< const Protocol::MsgRequestFile * >( imsg );
			Process( connection, msg ); 
			break;
		}
		case Protocol::MSG_SERVER_STATUS:
		{
			const Protocol::MsgServerStatus * msg = static_cast< const Protocol::MsgServerStatus * >( imsg );
			Process( connection, msg ); 
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
	FREE( (void *)( ss->m_CurrentMessage ) );
	FREE( payload );
	ss->m_CurrentMessage = nullptr;
}

// Process( MsgRequestJob )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestJob * )
{
    PROFILE_SECTION( "MsgRequestJob" )

	if ( JobQueue::IsValid() == false )
	{
		return;
	}

	ServerState * ss = (ServerState *)connection->GetUserData();
	ASSERT( ss );

	// no jobs for blacklisted workers
	if ( ss->m_Blacklisted )
	{
		MutexHolder mh( ss->m_Mutex );
		Protocol::MsgNoJobAvailable msg;
		msg.Send( connection );
		return;
	}

	Job * job = JobQueue::Get().GetDistributableJobToProcess( true );
	if ( job == nullptr )
	{
        PROFILE_SECTION( "NoJob" )
		// tell the client we don't have anything right now
		// (we completed or gave away the job already)
		MutexHolder mh( ss->m_Mutex );
		Protocol::MsgNoJobAvailable msg;
		msg.Send( connection );
		return;
	}

	// send the job to the client
	MemoryStream stream;
	job->Serialize( stream );

	MutexHolder mh( ss->m_Mutex );

	ss->m_Jobs.Append( job ); // Track in-flight job

	// if tool is explicity specified, get the id of the tool manifest
	Node * n = job->GetNode()->CastTo< ObjectNode >()->GetCompiler();
	const ToolManifest & manifest = n->CastTo< CompilerNode >()->GetManifest();
	uint64_t toolId = manifest.GetToolId();
	ASSERT( toolId );

	// output to signify remote start
	AStackString<> address; // TODO:B the host name would be better
	TCPConnectionPool::GetAddressAsString( connection->GetRemoteAddress(), address );
	FLOG_BUILD( "-> Obj: %s <REMOTE: %s>\n", job->GetNode()->GetName().Get(), address.Get() );

    {
        PROFILE_SECTION( "SendJob" )
	    Protocol::MsgJob msg( toolId );
	    msg.Send( connection, stream );
    }
}

// Process( MsgJobResult )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgJobResult *, const void * payload, size_t payloadSize )
{
    PROFILE_SECTION( "MsgJobResult" )

	// find server
	ServerState * ss = (ServerState *)connection->GetUserData();
	ASSERT( ss );

	ConstMemoryStream ms( payload, payloadSize );

	uint32_t jobId = 0;
	ms.Read( jobId );

	AStackString<> name;
	ms.Read( name );

	bool result = false;
	ms.Read( result );

	bool systemError = false;
	ms.Read( systemError );

	Array< AString > messages;
	ms.Read( messages );

	uint32_t buildTime;
	ms.Read( buildTime );

	// get result data (built data or errors if failed)
	uint32_t size = 0;
	ms.Read( size );
	const void * data = (const char *)ms.GetData() + ms.Tell();

	// manage job races
	bool cancelled( false );
	Job * job = JobQueue::Get().OnReturnRemoteJob( jobId, cancelled );

	{
		MutexHolder mh( ss->m_Mutex );
		Job ** iter = ss->m_Jobs.Find( job );
		ASSERT( iter );
		ss->m_Jobs.Erase( iter );
	}

	// has the job been cancelled in the interim?
	if ( cancelled )
	{
		// don't save result as we were cancelled
		return;
	}

	if ( result == true )
	{
		// built ok - serialize to disc

		ObjectNode * objectNode = job->GetNode()->CastTo< ObjectNode >();
		const AString & nodeName = objectNode->GetName();
		if ( Node::EnsurePathExistsForFile( nodeName ) == false )
		{
			FLOG_ERROR( "Failed to create path for '%s'", nodeName.Get() );
			result = false;
		}
		else
		{
			const ObjectNode * on = job->GetNode()->CastTo< ObjectNode >();
			const uint32_t firstFileSize = *(uint32_t *)data;
			const uint32_t secondFileSize = on->IsUsingPDB() ? *(uint32_t *)( (const char *)data + sizeof( uint32_t ) + firstFileSize ) : 0;

			FileStream fs;
			if ( fs.Open( nodeName.Get(), FileStream::WRITE_ONLY ) == false )
			{
				FLOG_ERROR( "Failed to create file '%s'", nodeName.Get() );
				result = false;
			}
			else if ( fs.WriteBuffer( (const char *)data + sizeof( uint32_t ), firstFileSize ) != firstFileSize )
			{
				FLOG_ERROR( "Failed to write file '%s'", nodeName.Get() );
				result = false;
			}
			else if ( on->IsUsingPDB() ) // is there a second file?
			{
				data = (const void *)( (const char *)data + sizeof( uint32_t ) + firstFileSize );
				ASSERT( ( firstFileSize + secondFileSize + ( sizeof( uint32_t ) * 2 ) ) == size );

				AStackString<> pdbName;
				on->GetPDBName( pdbName );
				FileStream fs2;
				if ( fs2.Open( pdbName.Get(), FileStream::WRITE_ONLY ) == false )
				{
					FLOG_ERROR( "Failed to create file '%s'", pdbName.Get() );
					result = false;
				}
				else if ( fs2.WriteBuffer( (const char *)data + sizeof( uint32_t ), secondFileSize ) != secondFileSize )
				{
					FLOG_ERROR( "Failed to write file '%s'", pdbName.Get() );
					result = false;
				}
			}

			if ( result == true )
			{
				// record build time
				fs.Close();
				FileNode * f = (FileNode *)job->GetNode();
				f->m_Stamp = FileIO::GetFileLastWriteTime( nodeName );

				// record time taken to build
				f->SetLastBuildTime( buildTime );
				f->SetStatFlag(Node::STATS_BUILT);
				f->SetStatFlag(Node::STATS_BUILT_REMOTE);

				// commit to cache?
				if ( FBuild::Get().GetOptions().m_UseCacheWrite &&
						objectNode->ShouldUseCache() )
				{
					objectNode->WriteToCache( job );
				}
			}
		}
	}
	else
	{
		// failed - build list of errors
		const AString & nodeName = job->GetNode()->GetName();
		AStackString< 8192 > failureOutput;
		failureOutput.Format( "PROBLEM: %s\n", nodeName.Get() );
		for ( const AString * it = messages.Begin(); it != messages.End(); ++it )
		{
			failureOutput += *it;
		}

		// was it a system error?
		if ( systemError )
		{
			// blacklist misbehaving worker
			ss->m_Blacklisted = true;

			// take note of failure of job
			job->OnSystemError();

			// debugging message
			const size_t workerIndex = ( ss - m_ServerList.Begin() );
			const AString & workerName = m_WorkerList[ workerIndex ];
			FLOG_INFO( "Remote System Failure!\n"
					   " - Blacklisted Worker: %s\n"
					   " - Node		         : %s\n"
					   " - Job Error Count   : %u / %u\n",
					   workerName.Get(),
					   job->GetNode()->GetName().Get(),
					   job->GetSystemErrorCount(), SYSTEM_ERROR_ATTEMPT_COUNT
					  );

			// should we retry on another worker?
			if ( job->GetSystemErrorCount() < SYSTEM_ERROR_ATTEMPT_COUNT )
			{
				// re-queue job which will be re-attempted on another worker
				JobQueue::Get().ReturnUnfinishedDistributableJob( job, systemError );
				return;
			}

			// failed too many times on different workers, add info about this to
			// error output
			AStackString<> tmp;
			tmp.Format( "FBuild: Error: Task failed on %u different workers\n", (uint32_t)SYSTEM_ERROR_ATTEMPT_COUNT );
			if ( failureOutput.EndsWith( '\n' ) == false )
			{
				failureOutput += '\n';
			}
			failureOutput += tmp;
		}

		Node::DumpOutput( nullptr, failureOutput.Get(), failureOutput.GetLength(), nullptr );
	}

	JobQueue::Get().FinishedProcessingJob( job, result, true, false ); // remote job, not a race of a remote job
}

// Process( MsgRequestManifest )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestManifest * msg )
{
    PROFILE_SECTION( "MsgRequestManifest" )

	// find a job associated with this client with this toolId
	const uint64_t toolId = msg->GetToolId();
	ASSERT( toolId );
	const ToolManifest * manifest = FindManifest( connection, toolId );

	if ( manifest == nullptr )
	{
		// client asked for a manifest that is not valid
		ASSERT( false ); // this indicates a logic bug
		Disconnect( connection );
		return;
	}

	MemoryStream ms;
	manifest->Serialize( ms );

	// Send manifest to worker
	Protocol::MsgManifest resultMsg( toolId );
	resultMsg.Send( connection, ms );
}

// Process ( MsgRequestFile )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgRequestFile * msg )
{
    PROFILE_SECTION( "MsgRequestFile" )

	// find a job associated with this client with this toolId
	const uint64_t toolId = msg->GetToolId();
	ASSERT( toolId != 0 ); // server should not request 'no sync' tool id
	const ToolManifest * manifest = FindManifest( connection, toolId );

	if ( manifest == nullptr )
	{
		// client asked for a manifest that is not valid
		ASSERT( false ); // this indicates a logic bug
		Disconnect( connection );
		return;
	}

	const uint32_t fileId = msg->GetFileId();
	size_t dataSize( 0 );
	const void * data = manifest->GetFileData( fileId, dataSize );
	if ( !data )
	{
		ASSERT( false ); // something is terribly wrong
		Disconnect( connection );
		return;
	}

	ConstMemoryStream ms( data, dataSize );

	// Send file to worker
	Protocol::MsgFile resultMsg( toolId, fileId );
	resultMsg.Send( connection, ms );
}

// Process ( MsgServerStatus )
//------------------------------------------------------------------------------
void Client::Process( const ConnectionInfo * connection, const Protocol::MsgServerStatus * msg )
{
    PROFILE_SECTION( "MsgServerStatus" )

	(void)msg;

	// find server
	ServerState * ss = (ServerState *)connection->GetUserData();
	ASSERT( ss );

	MutexHolder mh( ss->m_Mutex );
	ss->m_StatusTimer.Start();
}

// FindManifest
//------------------------------------------------------------------------------
const ToolManifest * Client::FindManifest( const ConnectionInfo * connection, uint64_t toolId ) const
{
	ServerState * ss = (ServerState *)connection->GetUserData();
	ASSERT( ss );

	MutexHolder mh( ss->m_Mutex );

	for ( Job ** it = ss->m_Jobs.Begin();
		  it != ss->m_Jobs.End();
		  ++it )
	{
		Node * n = ( *it )->GetNode()->CastTo< ObjectNode >()->GetCompiler();
		const ToolManifest & m = n->CastTo< CompilerNode >()->GetManifest();
		if ( m.GetToolId() == toolId )
		{
			// found a job with the same toolid
			return &m;
		}
	}

	return nullptr;
}

// CONSTRUCTOR( ServerState )
//------------------------------------------------------------------------------
Client::ServerState::ServerState() 
	: m_Connection( nullptr )
	, m_CurrentMessage( nullptr ) 
	, m_Jobs( 16, true )
	, m_Blacklisted( false )
{ 
	m_DelayTimer.Start( 999.0f );
}

//------------------------------------------------------------------------------
