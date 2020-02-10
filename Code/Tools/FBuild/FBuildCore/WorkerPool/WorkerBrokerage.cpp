// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerage.h"

#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Thread.h"
#include "Core/Time/Time.h"

// Constants
//------------------------------------------------------------------------------
static const float sBrokerageElapsedTimeBetweenClean = ( 12 * 60 * 60.0f );
static const uint32_t sBrokerageCleanOlderThan = ( 24 * 60 * 60 );

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::WorkerBrokerage()
    : m_Availability( false )
    , m_Initialized( false )
    , m_SettingsWriteTime( 0 )
{
}

// Init
//------------------------------------------------------------------------------
void WorkerBrokerage::Init()
{
    PROFILE_FUNCTION

    if ( m_Initialized )
    {
        return;
    }

    // brokerage path includes version to reduce unnecssary comms attempts
    uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

    // root folder
    AStackString<> brokeragePath;
    if ( Env::GetEnvVariable( "FASTBUILD_BROKERAGE_PATH", brokeragePath) )
    {
        // FASTBUILD_BROKERAGE_PATH can contain multiple paths separated by semi-colon. The worker will register itself into the first path only but
        // the additional paths are paths to additional broker roots allowed for finding remote workers(in order of priority)
        const char * start = brokeragePath.Get();
        const char * end = brokeragePath.GetEnd();
        AStackString<> pathSeparator( ";" );
        while ( true )
        {
            AStackString<> root;
            AStackString<> brokerageRoot;

            const char * separator = brokeragePath.Find( pathSeparator, start, end );
            if ( separator != nullptr )
            {
                root.Append( start, (size_t)( separator - start ) );
            }
            else
            {
                root.Append( start, (size_t)( end - start ) );
            }
            root.TrimStart( ' ' );
            root.TrimEnd( ' ' );
            // <path>/<group>/<version>/
            #if defined( __WINDOWS__ )
                brokerageRoot.Format( "%s\\main\\%u.windows\\", root.Get(), protocolVersion );
            #elif defined( __OSX__ )
                brokerageRoot.Format( "%s/main/%u.osx/", root.Get(), protocolVersion );
            #else
                brokerageRoot.Format( "%s/main/%u.linux/", root.Get(), protocolVersion );
            #endif

            m_BrokerageRoots.Append( brokerageRoot );
            if ( !m_BrokerageRootPaths.IsEmpty() )
            {
                m_BrokerageRootPaths.Append( pathSeparator );
            }

            m_BrokerageRootPaths.Append( brokerageRoot );

            if ( separator != nullptr )
            {
                start = separator + 1;
            }
            else
            {
                break;
            }
        }
    }

    Network::GetHostName(m_HostName);

    m_TimerLastUpdate.Start();
    m_TimerLastCleanBroker.Start( sBrokerageElapsedTimeBetweenClean ); // Set timer so we trigger right away

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
    const Array< AString > & paths,
    Array< AString > & contents ) const
{
    Array< AString > results( 256, true );
    size_t index = 0;
    for( const AString & path : paths )
    {
        FileIO::GetFiles(path,
                         AStackString<>( "*" ),
                         false,
                         &results );
        const size_t numResults = results.GetSize();
        contents.SetCapacity( numResults );
        for ( size_t i=0; i<numResults; ++i )
        {
            const char * lastSlash = results[ i ].FindLast( NATIVE_SLASH );
            AStackString<> machineName( lastSlash + 1 );
            contents.Append( machineName );
        }
        index += 1;
    }
}

// GetWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::GetWorkers(
    const Array< AString > & excludedWorkers,
    Array< AString > & workersToInclude ) const
{
    Array< AString > workers;
    ListDirContents( m_BrokerageRoots, workers );
    workersToInclude.SetCapacity( workers.GetSize() );
    for ( size_t i=0; i<workers.GetSize(); ++i )
    {
        const AString & worker = workers[ i ];
        if ( worker.CompareI( m_HostName ) != 0 &&
              !excludedWorkers.Find( worker ) )
        {
            if ( !workersToInclude.Find( worker ) )
            {
                workersToInclude.Append( worker );
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

    if ( m_BrokerageRoots.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?" );
        return;
    }

    GetWorkers( excludedWorkers, workers );
}

// SetBrokerageRecord
//------------------------------------------------------------------------------
void WorkerBrokerage::SetBrokerageRecord()
{
    if ( m_BrokerageRecord.m_DirPath.IsEmpty() )
    {
        // include a record for the brokerage root
        // will be searched for by client jobs
        AStackString<> brokerageFilePath( m_BrokerageRoots[0] );
        brokerageFilePath += NATIVE_SLASH;
        brokerageFilePath += m_HostName;
        m_BrokerageRecord.m_DirPath = m_BrokerageRoots[0];
        m_BrokerageRecord.m_FilePath = brokerageFilePath;
    }
}

// SetAvailable
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailable()
{
    Init();

    // ignore if brokerage not configured
    if ( m_BrokerageRoots.IsEmpty() )
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

    PeriodicCleanup();
}

// SetUnavailable
//------------------------------------------------------------------------------
void WorkerBrokerage::SetUnavailable()
{
    Init();

    // ignore if brokerage not configured
    if ( m_BrokerageRoots.IsEmpty() )
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

        // If settings have changed, (re)create the file 
        // If settings have not changed, update the modification timestamp
        const WorkerSettings & workerSettings = WorkerSettings::Get();
        const uint64_t settingsWriteTime = workerSettings.GetSettingsWriteTime();
        bool createBrokerageFile = ( settingsWriteTime > m_SettingsWriteTime );

        if ( createBrokerageFile == false )
        {
            // Update the modified time
            // (Allows an external process to delete orphaned files (from crashes/terminated workers)
            if ( FileIO::SetFileLastWriteTimeToNow( m_BrokerageRecord.m_FilePath ) == false )
            {
                // Failed to update time - try to create or recreate the file
                createBrokerageFile = true;
            }
        }

        if ( createBrokerageFile )
        {
            // Version
            AStackString<> buffer;
            buffer.AppendFormat( "Version: %s\n", FBUILD_VERSION_STRING );

            // Username
            AStackString<> userName;
            Env::GetLocalUserName( userName );
            buffer.AppendFormat( "User: %s\n", userName.Get() );

            // CPU Thresholds
            static const uint32_t numProcessors = Env::GetNumProcessors();
            buffer.AppendFormat( "CPUs: %u/%u\n", workerSettings.GetNumCPUsToUse(), numProcessors );

            // Move
            switch ( workerSettings.GetMode() )
            {
                case WorkerSettings::DISABLED:      buffer += "Mode: disabled\n";     break;
                case WorkerSettings::WHEN_IDLE:     buffer += "Mode: idle\n";         break;
                case WorkerSettings::DEDICATED:     buffer += "Mode: dedicated\n";    break;
                case WorkerSettings::PROPORTIONAL:  buffer += "Mode: proportional\n"; break;
            }

            // Create/write file which signifies availability
            FileIO::EnsurePathExists( m_BrokerageRecord.m_DirPath );
            FileStream fs;
            fs.Open( m_BrokerageRecord.m_FilePath.Get(), FileStream::WRITE_ONLY );
            fs.WriteBuffer( buffer.Get(), buffer.GetLength() );

            // Take note of time we wrote the settings
            m_SettingsWriteTime = settingsWriteTime;
        }

        // Restart the timer
        m_TimerLastUpdate.Start();
    }

    PeriodicCleanup();
}

// RemoveBrokerageFile
//------------------------------------------------------------------------------
void WorkerBrokerage::RemoveBrokerageFile()
{
    // Ensure the file disappears
    FileIO::FileDelete( m_BrokerageRecord.m_FilePath.Get() );
}

// PeriodicCleanup
//------------------------------------------------------------------------------
void WorkerBrokerage::PeriodicCleanup()
{
    // Handle brokereage cleaning
    if ( m_TimerLastCleanBroker.GetElapsed() >= sBrokerageElapsedTimeBetweenClean )
    {
        const uint64_t fileTimeNow = Time::FileTimeToSeconds( Time::GetCurrentFileTime() );

        Array< AString > files( 256, true );
        if ( !FileIO::GetFiles( m_BrokerageRoots[ 0 ],
                                AStackString<>( "*" ),
                                false,
                                &files ) )
        {
            FLOG_WARN( "No workers found in '%s' (or inaccessible)", m_BrokerageRoots[ 0 ].Get() );
        }

        for ( const AString & file : files )
        {
            const uint64_t lastWriteTime = Time::FileTimeToSeconds( FileIO::GetFileLastWriteTime( file ) );
            if ( ( fileTimeNow > lastWriteTime ) && ( ( fileTimeNow - lastWriteTime ) > sBrokerageCleanOlderThan ) )
            {
                FLOG_WARN( "Removing '%s' (too old)", file.Get() );
                FileIO::FileDelete( file.Get() );
            }
        }

        // Restart the timer
        m_TimerLastCleanBroker.Start();
    }    
}

//------------------------------------------------------------------------------
