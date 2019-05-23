// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerage.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Thread.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::WorkerBrokerage()
    : m_Availability( false )
    , m_Initialized( false )
{
}

// Init
//------------------------------------------------------------------------------
void WorkerBrokerage::Init()
{
    PROFILE_FUNCTION

    ASSERT( Thread::IsMainThread() );

    if ( m_Initialized )
    {
        return;
    }

    // brokerage path includes version to reduce unnecssary comms attempts
    uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

    // root folder
    AStackString<> root;
    if ( Env::GetEnvVariable( "FASTBUILD_BROKERAGE_PATH", root ) )
    {
        // <path>/<group>/<version>/
        #if defined( __WINDOWS__ )
            m_BrokerageRoot.Format( "%s\\main\\%u.windows\\", root.Get(), protocolVersion );
        #elif defined( __OSX__ )
            m_BrokerageRoot.Format( "%s/main/%u.osx/", root.Get(), protocolVersion );
        #else
            m_BrokerageRoot.Format( "%s/main/%u.linux/", root.Get(), protocolVersion );
        #endif
    }

    Network::GetHostName(m_HostName);

    m_TimerLastUpdate.Start();

    m_Initialized = true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::~WorkerBrokerage()
{
    RemoveBrokerageFile();
}

// ListDirContents
//------------------------------------------------------------------------------
void WorkerBrokerage::ListDirContents(
    const AString & path,
    Array< AString > & contents ) const
{
    // wildcard search for any worker
    // the filenames are the machine names
    AStackString<> wildcardFileName( "*" );
    Array< AString > pathResults( 256, true );  // start with 256 capacity
    FileIO::GetFiles( path,
                            wildcardFileName,  
                            false,  // recurse
                            &pathResults );
    const size_t numResults = pathResults.GetSize();
    contents.SetCapacity( numResults );
    for ( size_t i=0; i<numResults; ++i )
    {
        const char * lastSlash = pathResults[ i ].FindLast( NATIVE_SLASH );
        AStackString<> machineName( lastSlash + 1 );
        contents.Append( machineName );
    }
}

// GetRootWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::GetRootWorkers(
    const Array< AString > & excludedWorkers,
    Array< AString > & rootWorkersToInclude ) const
{
    Array< AString > rootWorkers;
    ListDirContents( m_BrokerageRoot, rootWorkers );
    rootWorkersToInclude.SetCapacity( rootWorkers.GetSize() );
    for ( size_t i=0; i<rootWorkers.GetSize(); ++i )
    {
        const AString & rootWorker = rootWorkers[ i ];
        if ( rootWorker.CompareI( m_HostName ) != 0 &&
              !excludedWorkers.Find( rootWorker ) )
        {
            if ( !rootWorkersToInclude.Find( rootWorker ) )
            {
                rootWorkersToInclude.Append( rootWorker );
            }
        }
    }
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers(
    const Array< AString > & excludedWorkers,
    Array< AString > & workers )
{
    PROFILE_FUNCTION

    Init();

    if ( m_BrokerageRoot.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?" );
        return;
    }

    GetRootWorkers( excludedWorkers, workers );
}

// SetBrokerageRecord
//------------------------------------------------------------------------------
void WorkerBrokerage::SetBrokerageRecord()
{
    if ( m_BrokerageRecord.m_DirPath.IsEmpty() )
    {
        // include a record for the brokerage root
        // will be searched for by client jobs
        AStackString<> brokerageFilePath( m_BrokerageRoot );
        brokerageFilePath += NATIVE_SLASH;
        brokerageFilePath += m_HostName;
        m_BrokerageRecord.m_DirPath = m_BrokerageRoot;
        m_BrokerageRecord.m_FilePath = brokerageFilePath;
    }
}

// SetAvailable
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailable()
{
    Init();

    // ignore if brokerage not configured
    if ( m_BrokerageRoot.IsEmpty() )
    {
        return;
    }

    bool updateFiles = true;
    if ( m_Availability )
    {
        // already available; so nothing changed, don't do updates
        // fall back to timer
        // check timer, to ensure that the files will be recreated if cleanup is done on the brokerage path
        updateFiles = m_TimerLastUpdate.GetElapsedMS() >= 10000.0f;
    }
    else  // not available
    {
        // so we can write the files immediately on worker launch, update all
        // update bool is true here, so just continue
    }
    if ( updateFiles )
    {
        SetBrokerageRecord();

        // create files to signify availability
        if ( !FileIO::FileExists( m_BrokerageRecord.m_FilePath.Get() ) )
        {
            FileStream fs;
            // create the dir path down to the file
            FileIO::EnsurePathExists( m_BrokerageRecord.m_DirPath );
            // create empty file; clients look at the dir names and filename,
            // not the contents of the file
            fs.Open( m_BrokerageRecord.m_FilePath.Get(), FileStream::WRITE_ONLY );
        }
        // Restart the timer
        m_TimerLastUpdate.Start();
        m_Availability = true;
    }
}

// SetUnavailable
//------------------------------------------------------------------------------
void WorkerBrokerage::SetUnavailable()
{
    Init();

    // ignore if brokerage not configured
    if ( m_BrokerageRoot.IsEmpty() )
    {
        return;
    }

    // if already available, then make unavailable
    if ( m_Availability )
    {
        // to remove availability, remove the files
        RemoveBrokerageFile();

        // Restart the timer
        m_TimerLastUpdate.Start();
        m_Availability = false;
    }
}

// RemoveBrokerageFile
//------------------------------------------------------------------------------
void WorkerBrokerage::RemoveBrokerageFile()
{
    // Ensure the file disappears
    FileIO::FileDelete( m_BrokerageRecord.m_FilePath.Get() );
}

//------------------------------------------------------------------------------
