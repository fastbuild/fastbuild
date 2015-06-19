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
	, m_Data( nullptr )
	, m_DataSize( 0 ) 
	, m_UserData( nullptr )
	, m_DataIsCompressed( false )
	, m_IsLocal( true )
	, m_SystemErrorCount( 0 )
	, m_ToolManifest( nullptr )
{
	m_JobId = AtomicIncU32( &s_LastJobId );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
Job::Job( IOStream & stream ) 
	: m_Node( nullptr )
	, m_Data( nullptr )
	, m_DataSize( 0 ) 
	, m_UserData( nullptr )
	, m_IsLocal( false )
	, m_SystemErrorCount( 0 )
	, m_ToolManifest( nullptr )
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
		FLOG_ERROR( buffer.Get() );
	}
	else
	{
		m_Messages.Append( buffer );
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

//------------------------------------------------------------------------------
