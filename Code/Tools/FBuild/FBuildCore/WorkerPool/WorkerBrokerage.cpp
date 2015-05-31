// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

#include "WorkerBrokerage.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::WorkerBrokerage()
	: m_Availability( false )
{
	// brokerage path includes version to reduce unnecssary comms attempts
	uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

	// root folder
	AStackString<> root;
	if ( Env::GetEnvVariable( "FASTBUILD_BROKERAGE_PATH", root ) )
    {
        // <path>/<group>/<version>/
        #if defined( __WINDOWS__ )
            m_BrokerageRoot.Format( "%s\\main\\%u\\", root.Get(), protocolVersion );
        #else
            m_BrokerageRoot.Format( "%s/main/%u/", root.Get(), protocolVersion );
        #endif
    }
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::~WorkerBrokerage()
{
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers( Array< AString > & workerList )
{
    PROFILE_FUNCTION

	if ( m_BrokerageRoot.IsEmpty() )
	{
		return;
	}

	Array< AString > results( 256, true );
	if ( !FileIO::GetFiles( m_BrokerageRoot,
							AStackString<>( "*" ),
							false,
							&results ) )
	{
		return; // no files found
	}

	// presize
	if ( ( workerList.GetSize() + results.GetSize() ) > workerList.GetCapacity() )
	{
		workerList.SetCapacity( workerList.GetSize() + results.GetSize() );
	}

	// we'll filter our own host if found
	AStackString<> hostName;
	Network::GetHostName( hostName );

	// convert worker strings
	const AString * const end = results.End();
	for ( AString * it = results.Begin(); it != end; ++it )
	{
		const AString & fileName = *it;
		const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
		AStackString<> workerName( lastSlash + 1 );
		if ( workerName.CompareI( hostName ) != 0 )
		{
			workerList.Append( workerName );
		}
	}
}

// SetAvailability
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailability( bool available )
{
	// ignore if brokerage not configured
	if ( m_BrokerageRoot.IsEmpty() )
	{
		return;
	}

	if ( m_Availability == available )
	{
		return; // avoid doing network IO when nothing has changed
	}
	m_Availability = available;

	FileIO::EnsurePathExists( m_BrokerageRoot );

	// Host Name - TODO:C Would it be better to use the IP here?
	AStackString<> hostName;
	Network::GetHostName( hostName );

	// construct filename : "ip.username.fastbuild"
	AStackString<> filePath;
	filePath.Format( "%s\\%s", m_BrokerageRoot.Get(), hostName.Get() );

	if ( available )
	{
		// create file to signify availability
		FileStream fs;
		fs.Open( filePath.Get(), FileStream::WRITE_ONLY );
	}
	else
	{
		// remove file to redact availability
		FileIO::FileDelete( filePath.Get() );
	}
}

//------------------------------------------------------------------------------
