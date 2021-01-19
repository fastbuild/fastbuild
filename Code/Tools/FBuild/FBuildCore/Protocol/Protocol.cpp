// Protocol.cpp - Defines network communication protocol
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Protocol.h"

#include "Core/Env/Env.h"
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Network/TCPConnectionPool.h"

// system
#include <memory.h> // for memset
#if defined( __APPLE__ ) || defined( __LINUX__ )
    #include <unistd.h> // for ::gethostname
#endif

// Message Debug
//------------------------------------------------------------------------------

#ifdef PROTOCOL_DEBUG_ENABLED
    const char * GetProtocolMessageDebugName( Protocol::MessageType msgType )
    {
        const char * const msgNames[] =
        {
            "",
            "Connection",
            "Status",
            "RequestJob",
            "NoJobAvailable",
            "Job",
            "JobResult",
            "RequestManifest",
            "Manifest",
            "RequestFile",
            "File",
        };
        static_assert( ( sizeof( msgNames ) / sizeof(const char *) ) == Protocol::NUM_MESSAGES, "msgNames item count doesn't match NUM_MESSAGES" );

        return msgNames[ msgType ];
    }
#endif

// IMessage
//------------------------------------------------------------------------------
Protocol::IMessage::IMessage( Protocol::MessageType msgType, uint8_t msgSize, bool hasPayload )
    : m_MsgType( msgType )
    , m_MsgSize( msgSize )
    , m_HasPayload( hasPayload )
{
    memset( m_Padding1, 0, sizeof( m_Padding1 ) );
}

// IMessage::Send
//------------------------------------------------------------------------------
bool Protocol::IMessage::Send( const ConnectionInfo * connection ) const
{
    ASSERT( connection );
    ASSERT( m_HasPayload == false ); // must use Send with payload

    // the packet is the contents of the derived class
    TCPConnectionPool & pool = connection->GetTCPConnectionPool();
    return pool.Send( connection, this, m_MsgSize );
}

// IMessage::Send (with payload)
//------------------------------------------------------------------------------
bool Protocol::IMessage::Send( const ConnectionInfo * connection, const MemoryStream & payload ) const
{
    ASSERT( connection );
    ASSERT( m_HasPayload == true ); // must NOT use Send with payload

    TCPConnectionPool & pool = connection->GetTCPConnectionPool();
    return pool.Send( connection, this, m_MsgSize, payload.GetData(), payload.GetSize() );
}

// IMessage::Send (with payload)
//------------------------------------------------------------------------------
bool Protocol::IMessage::Send( const ConnectionInfo * connection, const ConstMemoryStream & payload ) const
{
    ASSERT( connection );
    ASSERT( m_HasPayload == true ); // must NOT use Send with payload

    TCPConnectionPool & pool = connection->GetTCPConnectionPool();
    return pool.Send( connection, this, m_MsgSize, payload.GetData(), payload.GetSize() );
}

// IMessage::Broadcast
//------------------------------------------------------------------------------
bool Protocol::IMessage::Broadcast( TCPConnectionPool * pool ) const
{
    ASSERT( pool );
    ASSERT( m_HasPayload == false ); // must implement custom function

    // the packet is the contents of the derived class
    return pool->Broadcast( this, m_MsgSize );
}

// MsgConnection
//------------------------------------------------------------------------------
Protocol::MsgConnection::MsgConnection( uint32_t numJobsAvailable )
    : Protocol::IMessage( Protocol::MSG_CONNECTION, sizeof( MsgConnection ), false )
    , m_ProtocolVersion( PROTOCOL_VERSION )
    , m_NumJobsAvailable( numJobsAvailable )
    , m_Platform(Env::GetPlatform())
{
    memset( m_Padding2, 0, sizeof( m_Padding2 ) );
    memset( m_HostName, 0, sizeof( m_HostName ) );
    if ( ::gethostname( m_HostName, 64 ) != 0 )
    {
        AString::Copy( "Unavailable", m_HostName, 12 ); // inc terminator in copy
    }
}

// MsgStatus
//------------------------------------------------------------------------------
Protocol::MsgStatus::MsgStatus( uint32_t numJobsAvailable )
    : Protocol::IMessage( Protocol::MSG_STATUS, sizeof( MsgStatus ), false )
    , m_NumJobsAvailable( numJobsAvailable )
{
}

// MsgRequestJob
//------------------------------------------------------------------------------
Protocol::MsgRequestJob::MsgRequestJob()
    : Protocol::IMessage( Protocol::MSG_REQUEST_JOB, sizeof( MsgRequestJob ), false )
{
}

// MsgNoJobAvailable
//------------------------------------------------------------------------------
Protocol::MsgNoJobAvailable::MsgNoJobAvailable()
    : Protocol::IMessage( Protocol::MSG_NO_JOB_AVAILABLE, sizeof( MsgNoJobAvailable ), false )
{
}

// MsgJob
//------------------------------------------------------------------------------
Protocol::MsgJob::MsgJob( uint64_t toolId )
    : Protocol::IMessage( Protocol::MSG_JOB, sizeof( MsgJob ), true )
    , m_ToolId( toolId )
{
    memset( m_Padding2, 0, sizeof( m_Padding2 ) );
    ASSERT( toolId );
}

// MsgJobResult
//------------------------------------------------------------------------------
Protocol::MsgJobResult::MsgJobResult()
    : Protocol::IMessage( Protocol::MSG_JOB_RESULT, sizeof( MsgJobResult ), true )
{
}

// MsgRequestManifest
//------------------------------------------------------------------------------
Protocol::MsgRequestManifest::MsgRequestManifest( uint64_t toolId )
    : Protocol::IMessage( Protocol::MSG_REQUEST_MANIFEST, sizeof( MsgRequestManifest ), false )
    , m_ToolId( toolId )
{
    memset( m_Padding2, 0, sizeof( m_Padding2 ) );
}

// MsgManifest
//------------------------------------------------------------------------------
Protocol::MsgManifest::MsgManifest( uint64_t toolId )
    : Protocol::IMessage( Protocol::MSG_MANIFEST, sizeof( MsgManifest ), true )
    , m_ToolId( toolId )
{
    memset( m_Padding2, 0, sizeof( m_Padding2 ) );
}

// MsgRequestFile
//------------------------------------------------------------------------------
Protocol::MsgRequestFile::MsgRequestFile( uint64_t toolId, uint32_t fileId )
    : Protocol::IMessage( Protocol::MSG_REQUEST_FILE, sizeof( MsgRequestFile ), false )
    , m_FileId( fileId )
    , m_ToolId( toolId )
{
}

// MsgFile
//------------------------------------------------------------------------------
Protocol::MsgFile::MsgFile( uint64_t toolId, uint32_t fileId )
    : Protocol::IMessage( Protocol::MSG_FILE, sizeof( MsgFile ), true )
    , m_FileId( fileId )
    , m_ToolId( toolId )
{
}

//------------------------------------------------------------------------------
