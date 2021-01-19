// Job
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Job.h"

#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

#include "Core/Env/Assert.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/IOStream.h"
#include "Core/Process/Atomic.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"


// Static
//------------------------------------------------------------------------------
static uint32_t s_LastJobId( 0 );
/*static*/ int64_t Job::s_TotalLocalDataMemoryUsage( 0 );

// CONSTRUCTOR
//------------------------------------------------------------------------------
Job::Job( Node * node )
    : m_Node( node )
{
    m_JobId = AtomicIncU32( &s_LastJobId );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
Job::Job( IOStream & stream )
    : m_IsLocal( false )
{
    Deserialize( stream );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Job::~Job()
{
    if ( m_Data )
    {
        OwnData( nullptr, 0, false );
    }

    if ( m_IsLocal == false )
    {
        FDELETE m_Node;
    }

    ASSERT( m_BuildProfilerScope == nullptr ); // If set, must be unhooked
}

// Cancel
//------------------------------------------------------------------------------
void Job::Cancel()
{
    ASSERT( m_IsLocal ); // Cancellation should only occur locally
    ASSERT( m_Abort == false ); // Job must be not already be cancelled
    AtomicStoreRelaxed( &m_Abort, true );
}

// OwnData
//------------------------------------------------------------------------------
void Job::OwnData( void * data, size_t size, bool compressed )
{
    ASSERT( size <= 0xFFFFFFFF ); // only 32bit data supported
    ASSERT( data != m_Data ); // Invalid to set redundantly

    // Free any old data
    if ( m_Data )
    {
        FREE( m_Data );

        // Update total memory use tracking
        if ( m_IsLocal )
        {
            ASSERT( s_TotalLocalDataMemoryUsage >= m_DataSize );
            AtomicSub64( &s_TotalLocalDataMemoryUsage, (int32_t)m_DataSize );
        }
    }

    // Track new data
    m_Data = data;
    m_DataSize = (uint32_t)size;
    m_DataIsCompressed = compressed;

    // Update total memory use tracking
    if ( m_IsLocal )
    {
        AtomicAdd64( &s_TotalLocalDataMemoryUsage, (int32_t)m_DataSize );
    }
}

// Error
//------------------------------------------------------------------------------
void Job::Error( MSVC_SAL_PRINTF const char * format, ... )
{
    AStackString< 8192 > buffer;

    va_list args;
    va_start(args, format);
    buffer.VFormat( format, args );
    va_end( args );

    if ( IsLocal() )
    {
        FLOG_ERROR( "%s", buffer.Get() );

        if ( FLog::IsMonitorEnabled() )
        {
            m_Messages.Append( buffer );
        }
    }
    else
    {
        m_Messages.Append( buffer );
    }
}

// ErrorPreformatted
//------------------------------------------------------------------------------
void Job::ErrorPreformatted( const char * message )
{
    if ( IsLocal() )
    {
        FLOG_ERROR_DIRECT( message );

        if ( FLog::IsMonitorEnabled() )
        {
            m_Messages.EmplaceBack( message );
        }
    }
    else
    {
        m_Messages.EmplaceBack( message );
    }
}

// SetMessages
//------------------------------------------------------------------------------
void Job::SetMessages( const Array< AString > & messages )
{
    m_Messages = messages;
}

// Serialize
//------------------------------------------------------------------------------
void Job::Serialize( IOStream & stream )
{
    PROFILE_FUNCTION;

    // write jobid
    stream.Write( m_JobId );
    stream.Write( m_Node->GetName() );
    AStackString<> workingDir;
    VERIFY( FileIO::GetCurrentDir( workingDir ) );
    stream.Write( workingDir );

    // write properties of node
    Node::SaveRemote( stream, m_Node );

    stream.Write( IsDataCompressed() );

    stream.Write( m_DataSize );
    stream.Write( m_Data, m_DataSize );
}

// Deserialize
//------------------------------------------------------------------------------
void Job::Deserialize( IOStream & stream )
{
    // read jobid
    stream.Read( m_JobId );
    stream.Read( m_RemoteName );
    stream.Read( m_RemoteSourceRoot );

    // read properties of node
    m_Node = Node::LoadRemote( stream );

    bool compressed;
    stream.Read( compressed );

    // read extra data
    uint32_t dataSize;
    stream.Read( dataSize );
    void * data = ALLOC( dataSize );
    stream.Read( data, dataSize );

    OwnData( data, dataSize, compressed );
}

// GetMessagesForLog
//------------------------------------------------------------------------------
void Job::GetMessagesForLog( AString & buffer ) const
{
    // The common case is that there are no messages
    if ( m_Messages.IsEmpty() )
    {
        return;
    }

    // Ensure the output buffer is presized
    // (errors can sometimes be very large so we want to avoid re-allocs)
    uint32_t size( 0 );
    for ( const AString & msg : m_Messages )
    {
        size += msg.GetLength();
    }
    buffer.SetReserved( size ); // Will be safely ignored if smaller than already reserved

    // Concat the errors
    for( const AString & msg : m_Messages )
    {
        buffer += msg;
    }
}

// GetMessagesForMonitorLog
//------------------------------------------------------------------------------
void Job::GetMessagesForMonitorLog( AString & buffer ) const
{
    // The common case is that there are no messages
    if ( m_Messages.IsEmpty() )
    {
        return;
    }

    // concat all messages
    GetMessagesForLog( buffer );

    // Escape some characters to simplify parsing in the log
    // (The monitor will knows how to restore them)
    buffer.Replace( '\n', (char)12 );
    buffer.Replace( '\r', (char)12 );
    buffer.Replace( '\"', '\'' ); // TODO:B The monitor can't differentiate ' and "
}

// GetTotalLocalDataMemoryUsage
//------------------------------------------------------------------------------
/*static*/ uint64_t Job::GetTotalLocalDataMemoryUsage()
{
    return (uint64_t)AtomicLoadRelaxed( &s_TotalLocalDataMemoryUsage );
}

// SetBuildProfilerScope
//------------------------------------------------------------------------------
void Job::SetBuildProfilerScope( BuildProfilerScope * scope )
{
    // Only valid to have one scope at a time for a given Job
    ASSERT( ( m_BuildProfilerScope == nullptr ) || ( scope == nullptr ) );

    m_BuildProfilerScope = scope;
}

//------------------------------------------------------------------------------
