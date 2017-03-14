// Job
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "Job.h"

#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/Helpers/Compressor.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

#include "Core/Env/Assert.h"
#include "Core/FileIO/IOStream.h"
#include "Core/Process/Atomic.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"


// Static
//------------------------------------------------------------------------------
static uint32_t s_LastJobId( 0 );

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
    FREE( m_Data );

    if ( m_IsLocal == false )
    {
        FDELETE m_Node;
    }
}

// OwnData
//------------------------------------------------------------------------------
void Job::OwnData( void * data, size_t size, bool compressed )
{
    FREE( m_Data );

    ASSERT( size <= 0xFFFFFFFF ); // only 32bit data supported

    m_Data = data;
    m_DataSize = (uint32_t)size;
    m_DataIsCompressed = compressed;
}

// Error
//------------------------------------------------------------------------------
void Job::Error( const char * format, ... )
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
            m_Messages.Append( AStackString<>( message ) );
        }
    }
    else
    {
        m_Messages.Append( AStackString<>( message ) );
    }
}

// Serialize
//------------------------------------------------------------------------------
void Job::Serialize( IOStream & stream )
{
    PROFILE_FUNCTION

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

// GetMessagesForMonitorLog
//------------------------------------------------------------------------------
void Job::GetMessagesForMonitorLog( AString & buffer ) const
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

    // Escape some characters to simplify parsing in the log
    // (The monitor will knows how to restore them)
    buffer.Replace( '\n', (char)12 );
    buffer.Replace( '\r', (char)12 );
    buffer.Replace( '\"', '\'' ); // TODO:B The monitor can't differentiate ' and "
}

//------------------------------------------------------------------------------
