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
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Process/Thread.h"
#include "Core/Time/Time.h"

// Constants
//------------------------------------------------------------------------------
static const float sBrokerageElapsedTimeBetweenClean = ( 12 * 60 * 60.0f );
static const uint32_t sBrokerageCleanOlderThan = ( 24 * 60 * 60 );
static const float sBrokerageAvailabilityUpdateTime = ( 10.0f );
static const float sBrokerageIPAddressUpdateTime = ( 5 * 60.0f );

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::WorkerBrokerage()
    : m_Availability( false )
    , m_BrokerageInitialized( false )
    , m_SettingsWriteTime( 0 )
{
}

// InitBrokerage
//------------------------------------------------------------------------------
void WorkerBrokerage::InitBrokerage()
{
    PROFILE_FUNCTION;

    if ( m_BrokerageInitialized )
    {
        return;
    }

    // brokerage path includes version to reduce unnecessary comms attempts
    uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

    // root folder
    AStackString<> brokeragePath;
    if ( Env::GetEnvVariable( "FASTBUILD_BROKERAGE_PATH", brokeragePath ) )
    {
        // FASTBUILD_BROKERAGE_PATH can contain multiple paths separated by semi-colon. The worker will register itself into the first path only but
        // the additional paths are paths to additional broker roots allowed for finding remote workers (in order of priority)
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

    Network::GetHostName( m_HostName );

    UpdateBrokerageFilePath();

    m_TimerLastUpdate.Start();
    m_TimerLastIPUpdate.Start();
    m_TimerLastCleanBroker.Start( sBrokerageElapsedTimeBetweenClean ); // Set timer so we trigger right away

    m_BrokerageInitialized = true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::~WorkerBrokerage()
{
    // Ensure the file disappears when closing
    if ( m_Availability )
    {
        FileIO::FileDelete( m_BrokerageFilePath.Get() );
    }
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers( Array< AString > & workerList )
{
    PROFILE_FUNCTION;

    // Check for workers for the FASTBUILD_WORKERS environment variable
    // which is a list of worker addresses separated by a semi-colon.
    AStackString<> workersEnv;
    if ( Env::GetEnvVariable( "FASTBUILD_WORKERS", workersEnv ) )
    {
        // If we find a valid list of workers, we'll use that
        workersEnv.Tokenize( workerList, ';' );
        if ( workerList.IsEmpty() == false )
        {
            return;
        }
    }

    // check for workers through brokerage

    // Init the brokerage
    InitBrokerage();
    if ( m_BrokerageRoots.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?" );
        return;
    }

    Array< AString > results( 256, true );
    for( AString& root : m_BrokerageRoots )
    {
        size_t filesBeforeSearch = results.GetSize();
        if ( !FileIO::GetFiles( root,
                                AStackString<>( "*" ),
                                false,
                                &results ) )
        {
            FLOG_WARN( "No workers found in '%s'", root.Get() );
        }
        else
        {
            FLOG_WARN( "%zu workers found in '%s'", results.GetSize() - filesBeforeSearch, root.Get() );
        }
    }

    // presize
    if ( ( workerList.GetSize() + results.GetSize() ) > workerList.GetCapacity() )
    {
        workerList.SetCapacity( workerList.GetSize() + results.GetSize() );
    }

    // convert worker strings
    const AString * const end = results.End();
    for ( AString * it = results.Begin(); it != end; ++it )
    {
        const AString & fileName = *it;
        const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
        AStackString<> workerName( lastSlash + 1 );
        if ( workerName.CompareI( m_HostName ) != 0 )
        {
            workerList.Append( workerName );
        }
    }
}

// SetAvailability
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailability( bool available )
{
    // Init the brokerage if not already
    InitBrokerage();

    // ignore if brokerage not configured
    if ( m_BrokerageRoots.IsEmpty() )
    {
        return;
    }

    if ( available )
    {
        // Check the last update time to avoid too much File IO.
        float elapsedTime = m_TimerLastUpdate.GetElapsed();
        if ( elapsedTime >= sBrokerageAvailabilityUpdateTime )
        {
            // If settings have changed, (re)create the file 
            // If settings have not changed, update the modification timestamp
            const WorkerSettings & workerSettings = WorkerSettings::Get();
            const uint64_t settingsWriteTime = workerSettings.GetSettingsWriteTime();
            bool createBrokerageFile = ( settingsWriteTime > m_SettingsWriteTime );

            // Check IP last update time and determine if host name or IP address has changed
            if ( m_IPAddress.IsEmpty() || ( m_TimerLastIPUpdate.GetElapsed() >= sBrokerageIPAddressUpdateTime ) )
            {
                AStackString<> hostName;
                AStackString<> domainName;
                AStackString<> ipAddress;

                // Get host and domain name as FQDN could have changed
                Network::GetHostName( hostName );
                Network::GetDomainName( domainName );

                // Resolve host name to ip address
                uint32_t ip = Network::GetHostIPFromName( hostName );
                if ( ( ip != 0 ) && ( ip != 0x0100007f ) )
                {
                    TCPConnectionPool::GetAddressAsString( ip, ipAddress );
                }

                if ( ( hostName != m_HostName ) || ( domainName != m_DomainName ) || ( ipAddress != m_IPAddress ) )
                {
                    m_HostName = hostName;
                    m_DomainName = domainName;
                    m_IPAddress = ipAddress;

                    // Remove existing brokerage file, as filename is being updated
                    FileIO::FileDelete( m_BrokerageFilePath.Get() );

                    // Update brokerage path
                    UpdateBrokerageFilePath();

                    // Host name, domain name, or IP address changed - create the file
                    createBrokerageFile = true;
                }

                // Restart the IP timer
                m_TimerLastIPUpdate.Start();
            }

            if ( createBrokerageFile == false )
            {
                // Update the modified time
                // (Allows an external process to delete orphaned files (from crashes/terminated workers)
                if ( FileIO::SetFileLastWriteTimeToNow( m_BrokerageFilePath ) == false )
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

                // Host Name
                buffer.AppendFormat( "Host Name: %s\n", m_HostName.Get() );

                if ( !m_DomainName.IsEmpty() )
                {
                    // Domain Name
                    buffer.AppendFormat( "Domain Name: %s\n", m_DomainName.Get() );

                    // Fully Quantified Domain Name
                    buffer.AppendFormat( "FQDN: %s.%s\n", m_HostName.Get(), m_DomainName.Get() );
                }

                // IP Address
                buffer.AppendFormat( "IPv4 Address: %s\n", m_IPAddress.Get() );

                // CPU Thresholds
                static const uint32_t numProcessors = Env::GetNumProcessors();
                buffer.AppendFormat( "CPUs: %u/%u\n", workerSettings.GetNumCPUsToUse(), numProcessors );

                // Memory Threshold
                buffer.AppendFormat( "Memory: %u\n", workerSettings.GetMinimumFreeMemoryMiB() );

                // Mode
                switch ( workerSettings.GetMode() )
                {
                    case WorkerSettings::DISABLED:      buffer += "Mode: disabled\n";     break;
                    case WorkerSettings::WHEN_IDLE:     buffer.AppendFormat( "Mode: idle @ %u%%\n", workerSettings.GetIdleThresholdPercent() ); break;
                    case WorkerSettings::DEDICATED:     buffer += "Mode: dedicated\n";    break;
                    case WorkerSettings::PROPORTIONAL:  buffer += "Mode: proportional\n"; break;
                }

                // Create/write file which signifies availability
                FileIO::EnsurePathExists( m_BrokerageRoots[0] );
                FileStream fs;
                if ( fs.Open( m_BrokerageFilePath.Get(), FileStream::WRITE_ONLY ) )
                {
                    fs.WriteBuffer( buffer.Get(), buffer.GetLength() );

                    // Take note of time we wrote the settings
                    m_SettingsWriteTime = settingsWriteTime;
                }
            }

            // Restart the timer
            m_TimerLastUpdate.Start();
        }
    }
    else if ( m_Availability != available )
    {
        // remove file to remove availability
        FileIO::FileDelete( m_BrokerageFilePath.Get() );

        // Restart the timer
        m_TimerLastUpdate.Start();
    }
    m_Availability = available;
    
    // Handle brokerage cleaning
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
// UpdateBrokerageFilePath
//------------------------------------------------------------------------------
void WorkerBrokerage::UpdateBrokerageFilePath()
{
    if ( !m_BrokerageRoots.IsEmpty() )
    {
        if ( !m_IPAddress.IsEmpty() )
        {
            m_BrokerageFilePath.Format( "%s%s", m_BrokerageRoots[0].Get(), m_IPAddress.Get() );
        }
        else
        {
            m_BrokerageFilePath.Format( "%s%s", m_BrokerageRoots[0].Get(), m_HostName.Get() );
        }
    }
}

//------------------------------------------------------------------------------
