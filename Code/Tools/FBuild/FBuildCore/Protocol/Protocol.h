// Protocol.h - Defines network communication protocol
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Env/Types.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ConnectionInfo;
class ConstMemoryStream;
class MemoryStream;
class TCPConnectionPool;

// Defines
//------------------------------------------------------------------------------
//#define PROTOCOL_DEBUG_ENABLED // uncomment this for protocol spam
#ifdef PROTOCOL_DEBUG_ENABLED
    #include "Core/Tracing/Tracing.h"
    #define PROTOCOL_DEBUG( ... ) DEBUGSPAM( __VA_ARGS__ )
#else
    #define PROTOCOL_DEBUG( ... ) (void)0
#endif

// Protocol
//------------------------------------------------------------------------------
namespace Protocol
{
    enum { PROTOCOL_PORT = 31264 }; // Arbitrarily chosen port
    enum { PROTOCOL_VERSION = 18 };

    enum { SERVER_STATUS_FREQUENCY_MS = 1000 }; // frequency of server status updates to client
    enum { SERVER_STATUS_TIMEOUT_MS = 30000 };  // server is dead if time elapses between updates

    // Identifiers for all unique messages
    //------------------------------------------------------------------------------
    enum MessageType
    {
        MSG_CONNECTION          = 1, // Server <- Client : Initial handshake
        MSG_STATUS              = 2, // Server <- Client : Update status (work available)

        MSG_REQUEST_JOB         = 3, // Server -> Client : Ask for a job to do
        MSG_NO_JOB_AVAILABLE    = 4, // Server <- Client : Respond that no jobs are available
        MSG_JOB                 = 5, // Server <- Client : Respond with a job to do

        MSG_JOB_RESULT          = 6, // Server -> Client : Return completed job

        MSG_REQUEST_MANIFEST    = 7, // Server -> Client : Ask client for the manifest of tools required for a job
        MSG_MANIFEST            = 8, // Server <- Client : Respond with manifest details

        MSG_REQUEST_FILE        = 9, // Server -> Client : Ask client for a file
        MSG_FILE                = 10,// Server <- Client : Send a requested file

        MSG_SERVER_STATUS       = 11,// Server -> Client : Send status / keep connection alive

        NUM_MESSAGES            // leave last
    };
};

#ifdef PROTOCOL_DEBUG_ENABLED
    const char * GetProtocolMessageDebugName( Protocol::MessageType msgType );
#endif

namespace Protocol
{
    // base class for all messages
    //------------------------------------------------------------------------------
    class IMessage
    {
    public:
        bool Send( const ConnectionInfo * connection ) const;
        bool Send( const ConnectionInfo * connection, const MemoryStream & payload ) const;
        bool Send( const ConnectionInfo * connection, const ConstMemoryStream & payload ) const;
        bool Broadcast( TCPConnectionPool * pool ) const;

        inline MessageType  GetType() const { return m_MsgType; }
        inline uint32_t     GetSize() const { return m_MsgSize; }
        inline bool         HasPayload() const { return m_HasPayload; }

    protected:
        IMessage( MessageType msgType, uint32_t msgSize, bool hasPayload );

        // properties common to all messages
        MessageType     m_MsgType;
        uint32_t        m_MsgSize;
        bool            m_HasPayload;
        char            m_Padding1[ 3 ];
    };
    static_assert( sizeof( IMessage ) == 9 + 3/*padding*/, "Message base class has incorrect size" );

    // MsgConnection
    //------------------------------------------------------------------------------
    class MsgConnection : public IMessage
    {
    public:
        explicit MsgConnection( uint32_t numJobsAvailable );

        inline uint32_t GetProtocolVersion() const { return m_ProtocolVersion; }
        inline uint32_t GetNumJobsAvailable() const { return m_NumJobsAvailable; }
        const char * GetHostName() const { return m_HostName; }
    private:
        uint32_t        m_ProtocolVersion;
        uint32_t        m_NumJobsAvailable;
        char            m_HostName[ 64 ];
    };
    static_assert( sizeof( MsgConnection ) == sizeof( IMessage ) + 72, "MsgConnection message has incorrect size" );

    // MsgStatus
    //------------------------------------------------------------------------------
    class MsgStatus : public IMessage
    {
    public:
        explicit MsgStatus( uint32_t numJobsAvailable );

        inline uint32_t GetNumJobsAvailable() const { return m_NumJobsAvailable; }
    private:
        uint32_t        m_NumJobsAvailable;
    };
    static_assert( sizeof( MsgStatus ) == sizeof( IMessage ) + 4, "MsgStatus message has incorrect size" );

    // MsgRequestJob
    //------------------------------------------------------------------------------
    class MsgRequestJob : public IMessage
    {
    public:
        MsgRequestJob();
    };
    static_assert( sizeof( MsgRequestJob ) == sizeof( IMessage ), "MsgRequestJob message has incorrect size" );

    // MsgNoJobAvailable
    //------------------------------------------------------------------------------
    class MsgNoJobAvailable : public IMessage
    {
    public:
        MsgNoJobAvailable();
    };
    static_assert( sizeof( MsgNoJobAvailable ) == sizeof( IMessage ), "MsgNoJobAvailable message has incorrect size" );

    // MsgJob
    //------------------------------------------------------------------------------
    class MsgJob : public IMessage
    {
    public:
        explicit MsgJob( uint64_t toolId );

        inline uint64_t GetToolId() const { return m_ToolId; }
    private:
        char     m_Padding2[ 4 ];
        uint64_t m_ToolId;
    };
    static_assert( sizeof( MsgJob ) == sizeof( IMessage ) + 4/*alignment*/ + 8, "MsgJob message has incorrect size" );

    // MsgJobResult
    //------------------------------------------------------------------------------
    class MsgJobResult : public IMessage
    {
    public:
        MsgJobResult();
    };
    static_assert( sizeof( MsgJobResult ) == sizeof( IMessage ), "MsgJobResult message has incorrect size" );

    // MsgRequestManifest
    //------------------------------------------------------------------------------
    class MsgRequestManifest : public IMessage
    {
    public:
        explicit MsgRequestManifest( uint64_t toolId );

        inline uint64_t GetToolId() const { return m_ToolId; }
    private:
        char     m_Padding2[ 4 ];
        uint64_t m_ToolId;
    };
    static_assert( sizeof( MsgRequestManifest ) == sizeof( IMessage ) + 4/*alignment*/ + 8, "MsgRequestManifest message has incorrect size" );

    // MsgManifest
    //------------------------------------------------------------------------------
    class MsgManifest : public IMessage
    {
    public:
        explicit MsgManifest( uint64_t toolId );

        inline uint64_t GetToolId() const { return m_ToolId; }
    private:
        char     m_Padding2[ 4 ];
        uint64_t m_ToolId;
    };
    static_assert( sizeof( MsgManifest ) == sizeof( IMessage ) + 4/*alignment*/ + 8, "MsgManifest message has incorrect size" );

    // MsgRequestFile
    //------------------------------------------------------------------------------
    class MsgRequestFile : public IMessage
    {
    public:
        MsgRequestFile( uint64_t toolId, uint32_t fileId );

        inline uint64_t GetToolId() const { return m_ToolId; }
        inline uint32_t GetFileId() const { return m_FileId; }
    private:
        char     m_Padding2[ 4 ];
        uint64_t m_ToolId;
        uint32_t m_FileId;
        char     m_Padding3[ 4 ];
    };
    static_assert( sizeof( MsgRequestFile ) == sizeof( IMessage ) + 4/*alignment*/ + 12 + 4/*padding*/, "MsgRequestFile message has incorrect size" );

    // MsgFile
    //------------------------------------------------------------------------------
    class MsgFile : public IMessage
    {
    public:
        MsgFile( uint64_t toolId, uint32_t fileId );

        inline uint64_t GetToolId() const { return m_ToolId; }
        inline uint32_t GetFileId() const { return m_FileId; }
    private:
        char     m_Padding2[ 4 ];
        uint64_t m_ToolId;
        uint32_t m_FileId;
        char     m_Padding3[ 4 ];
    };
    static_assert( sizeof( MsgFile ) == sizeof( IMessage ) + 4/*alignment*/ + 12 + 4/*padding*/, "MsgFile message has incorrect size" );

    // MsgServerStatus
    //------------------------------------------------------------------------------
    class MsgServerStatus : public IMessage
    {
    public:
        MsgServerStatus();
    };
    static_assert( sizeof( MsgServerStatus ) == sizeof( IMessage ), "MsgServerStatus message has incorrect size" );
};

//------------------------------------------------------------------------------
