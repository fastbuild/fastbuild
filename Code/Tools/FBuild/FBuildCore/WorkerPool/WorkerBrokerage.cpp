// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerage.h"

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

    if ( !m_BrokerageRoots.IsEmpty() )
    {
        m_BrokerageFilePath.Format( "%s%s", m_BrokerageRoots[0].Get(), m_HostName.Get() );
    }
    m_TimerLastUpdate.Start();

    m_Initialized = true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::~WorkerBrokerage()
{
    // Ensure the file disapears when closing
    if ( m_Availability )
    {
        FileIO::FileDelete( m_BrokerageFilePath.Get() );
    }
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers( Array< AString > & workerList )
{
    PROFILE_FUNCTION

    Init();

    if ( m_BrokerageRoots.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?" );
        return;
    }

    Array< AString > results( 256, true );
    for( AString& root : m_BrokerageRoots )
    {
        size_t filesBeforeSearch = results.GetSize();
        if ( !FileIO::GetFiles(root,
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
void WorkerBrokerage::SetAvailability(bool available)
{
    Init();

    // ignore if brokerage not configured
    if ( m_BrokerageRoots.IsEmpty() )
    {
        return;
    }

    if ( available )
    {
        // Check the last update time to avoid too much File IO.
        float elapsedTime = m_TimerLastUpdate.GetElapsedMS();
        if ( elapsedTime >= 10000.0f )
        {
            bool createBrokerageFile = false;

            // Periodically update the modified time of brokerage file for workers that are still active,
            // so an external script can easily delete older brokerage files (clean up orphaned ones
            // from crashed workers).
            const bool timeSetOK = FileIO::SetFileLastWriteTimeToNow(
                m_BrokerageFilePath );
            if ( !timeSetOK )
            {
                if ( !FileIO::FileExists( m_BrokerageFilePath.Get() ) )
                {
                    // create the dir path down to the file
                    FileIO::EnsurePathExists( m_BrokerageRoots[0] );
                    createBrokerageFile = true;
                }
                else
                {
                    // failed to set write timestamp
                    return;
                }
            }

            // Write file if:
            // - missing
            // - settings have changed
            const WorkerSettings & workerSettings = WorkerSettings::Get();
            const uint64_t settingsWriteTime = workerSettings.GetSettingsWriteTime();
            if ( createBrokerageFile ||
                 ( settingsWriteTime > m_SettingsWriteTime ) )
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
                FileIO::EnsurePathExists( m_BrokerageRoots[0] );
                FileStream fs;
                fs.Open( m_BrokerageFilePath.Get(), FileStream::WRITE_ONLY );
                fs.WriteBuffer( buffer.Get(), buffer.GetLength() );

                // Take note of time we wrote the settings
                m_SettingsWriteTime = settingsWriteTime;

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
}

//------------------------------------------------------------------------------
