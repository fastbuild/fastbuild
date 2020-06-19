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
    , m_SettingsWriteTime( 0 )
{
    m_Status.statusValue = BrokerageUninitialized;
}

// Init
//------------------------------------------------------------------------------
void WorkerBrokerage::Init()
{
    PROFILE_FUNCTION

    if ( m_Status.statusValue != BrokerageUninitialized )
    {
        return;
    }

    // brokerage path includes version to reduce unnecssary comms attempts
    uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

    Network::GetHostName(m_HostName);

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

    m_TimerLastUpdate.Start();
    m_TimerLastCleanBroker.Start( sBrokerageElapsedTimeBetweenClean ); // Set timer so we trigger right away

    WorkerInfo workerInfo;
    workerInfo.basePath = "";
    workerInfo.name = m_HostName;

    if ( !m_BrokerageRoots.IsEmpty() )
    {
        // the worker only uses the first path in the roots
        workerInfo.basePath = m_BrokerageRoots[0];

        m_BrokerageFilePath.Format( "%s%s", workerInfo.basePath.Get(), m_HostName.Get() );

        AStackString<> denylistFile;
        if ( GetDenylistFile( workerInfo, denylistFile ) &&
             FileIO::FileExists( denylistFile.Get() ) )
        {
            StatusFailure statusFailure;
            statusFailure.workerInfo = workerInfo;
            statusFailure.failureValue = FailureDenylistedWorker;
            m_Status.failures.Append( statusFailure );
        }
        else
        {                
            FileIO::EnsurePathExists( workerInfo.basePath );
            if ( !FileIO::DirectoryExists( workerInfo.basePath ) )
            {
                StatusFailure statusFailure;
                statusFailure.workerInfo = workerInfo;
                statusFailure.failureValue = FailureNoBrokerageAccess;
                m_Status.failures.Append( statusFailure );
            }
        }
    }
    else
    {
        // no brokerage path
        StatusFailure statusFailure;
        statusFailure.workerInfo = workerInfo;
        statusFailure.failureValue = FailureNoBrokeragePath;
        m_Status.failures.Append( statusFailure );
    }
    
    if ( m_Status.failures.IsEmpty() )
    {
        m_Status.statusValue = BrokerageSuccess;
    }
    else
    {
        m_Status.statusValue = BrokerageFailure;
    }
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

// ListDirContents
//------------------------------------------------------------------------------
void WorkerBrokerage::ListDirContents(
    const Array< AString > & roots,
    const Array< AString > & paths,
    Array< WorkerInfo > & contents ) const
{
    Array< AString > results( 256, true );
    size_t index = 0;
    for( const AString & path : paths )
    {
        const AString & root = roots[ index ];
        
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
            WorkerInfo workerInfo;
            workerInfo.basePath = root;
            workerInfo.name = machineName;
            contents.Append( workerInfo );
        }
        index += 1;
    }
}

// GetStatusMsg
//------------------------------------------------------------------------------
void WorkerBrokerage::GetStatusMsg(
    const Status & status,
    AString & statusMsg ) const
{
    if (status.statusValue == BrokerageFailure)
    {
        for( const StatusFailure & statusFailure : status.failures )
        {
            switch( statusFailure.failureValue )
            {
                case FailureNoBrokeragePath:
                    statusMsg = "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?";
                    break;
                case FailureNoBrokerageAccess:
                    statusMsg.Format( "No access to brokerage root %s",
                        statusFailure.workerInfo.basePath.Get() );
                    break;
                case FailureDenylistedWorker:
                    {
                        statusMsg = "Your worker has been deny listed.";
                        statusMsg += " Please use a different brokerage path and debug your machine issue.";
                        AStackString<> denylistFile;
                        if ( GetDenylistFile(
                                statusFailure.workerInfo,
                                denylistFile ) )
                        {
                            statusMsg += " For issue details, see contents of ";
                            statusMsg += denylistFile;
                        }
                    }
                    break;
                default:
                    ASSERT( false ); // should never get here
            }
        }
    }
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers( Array< WorkerInfo > & workerList )
{
    PROFILE_FUNCTION

    Init();

    if ( m_Status.statusValue != WorkerBrokerage::BrokerageSuccess )
    {
        return;
    }

    if ( m_BrokerageRoots.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?" );
        return;
    }

    Array< WorkerInfo > results;
    ListDirContents(
        m_BrokerageRoots,  // roots
        m_BrokerageRoots,  // paths: in this case same as roots
        results );
    if ( results.IsEmpty() )
    {
        FLOG_WARN( "No workers found in '%s'", m_BrokerageRootPaths.Get() );
    }

    // presize
    if ( ( workerList.GetSize() + results.GetSize() ) > workerList.GetCapacity() )
    {
        workerList.SetCapacity( workerList.GetSize() + results.GetSize() );
    }

    RefreshDenylistedWorkers();

    const WorkerInfo * const end = results.End();
    for ( WorkerInfo * it = results.Begin(); it != end; ++it )
    {
        const WorkerInfo & worker = *it;
        if ( worker.name.CompareI( m_HostName ) != 0 &&
             !m_DenylistedWorkers.Find( worker ) )
        {
            workerList.Append( worker );
        }
    }
}

// RefreshDenylistedWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::RefreshDenylistedWorkers()
{
    Array< AString > denylistDirs( 256, true );
    for( AString& root : m_BrokerageRoots )
    {
        AStackString<> denylistDir( root );
        denylistDir += "denylist";
        denylistDirs.Append( denylistDir );
    }

    m_DenylistedWorkers.Clear();
    ListDirContents(
        m_BrokerageRoots,  // roots
        denylistDirs,      // paths
        m_DenylistedWorkers );
}

// DenylistWorker
//------------------------------------------------------------------------------
void WorkerBrokerage::DenylistWorker(
    const WorkerInfo & workerInfo,
    const AString & denylistReason,
    AString & errorMsg )
{
    if ( m_Status.statusValue != WorkerBrokerage::BrokerageSuccess )
    {
        return;
    }

    if ( !m_DenylistedWorkers.Find( workerInfo ) )
    {
        m_DenylistedWorkers.Append( workerInfo );
    }

    // only write to the one brokerage root we have the info for
    if ( !workerInfo.basePath.IsEmpty() )
    {
        AStackString<> denylistDir( workerInfo.basePath );
        denylistDir += "denylist";

        AStackString<> fileContents( "Client Machine: " );
        fileContents += m_HostName;
        fileContents += "\nClient User: ";
        if ( m_UserName.IsEmpty() )
        {
            Env::GetLocalUserName( m_UserName );
        }
        fileContents += m_UserName;
        fileContents += "\n";
        fileContents += denylistReason;

        AStackString<> denylistFile;
        if ( GetDenylistFile( workerInfo, denylistFile ) && 
             !FileIO::FileExists( denylistFile.Get() ) )
        {
            FileStream fs;
            // create the dir path down to the file
            FileIO::EnsurePathExists( denylistDir );
            if ( fs.Open( denylistFile.Get(), FileStream::WRITE_ONLY ) == false )
            {
                errorMsg.Format( "Failed to open deny list file %s for writing", denylistFile.Get() );
                return;
            }
            if ( fs.Write( fileContents.Get(), fileContents.GetLength() ) != fileContents.GetLength() )
            {
                errorMsg.Format( "Failed to write to deny list file %s", denylistFile.Get() );
                return;
            }
            fs.Close();
        }
    }
}

// GetDenylistFile
//------------------------------------------------------------------------------
bool WorkerBrokerage::GetDenylistFile(
    const WorkerInfo & workerInfo,
    AString & filePath ) const
{
    if ( workerInfo.basePath.IsEmpty() )
    {
        return false;
    }
    
    filePath.Assign( workerInfo.basePath );
    filePath += "denylist";
    filePath += NATIVE_SLASH;
    filePath += workerInfo.name;
    return true;
}

// ShouldUpdateAvailability
//------------------------------------------------------------------------------
bool WorkerBrokerage::ShouldUpdateAvailability() const
{
    bool shouldUpdateAvailability = false;

    // update availability if BrokerageSuccess or DenylistedWorker
    if ( m_Status.statusValue == WorkerBrokerage::BrokerageSuccess )
    {
        shouldUpdateAvailability = true;
    }
    else if ( m_Status.statusValue == WorkerBrokerage::BrokerageFailure )
    {
        if ( !m_Status.failures.IsEmpty() )
        {
            if ( m_Status.failures[0].failureValue ==
                 WorkerBrokerage::FailureDenylistedWorker )
            {
                shouldUpdateAvailability = true;
            }
        }
    }
    return shouldUpdateAvailability;
}

// SetAvailability
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailability(bool available)
{
    Init();

    if ( !ShouldUpdateAvailability() )
    {
        return;
    }

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
            // the worker only uses the first path in the roots
            WorkerInfo workerInfo;
            workerInfo.basePath = m_BrokerageRoots[0];
            workerInfo.name = m_HostName;

            AStackString<> denylistFile;
            if ( GetDenylistFile( workerInfo, denylistFile ) &&
                 FileIO::FileExists( denylistFile.Get() ) )
            {
                if ( m_Status.failures.IsEmpty() )
                {
                    StatusFailure statusFailure;
                    statusFailure.workerInfo = workerInfo;
                    statusFailure.failureValue = FailureDenylistedWorker;
                    m_Status.failures.Append( statusFailure );
                }
                else
                {
                    // we already have a failure, so skip this one
                    // to avoid growing m_Status.failures unbounded
                }
            }
            else
            {
                // If settings have changed, (re)create the file 
                // If settings have not changed, update the modification timestamp
                const WorkerSettings & workerSettings = WorkerSettings::Get();
                const uint64_t settingsWriteTime = workerSettings.GetSettingsWriteTime();
                bool createBrokerageFile = ( settingsWriteTime > m_SettingsWriteTime );

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

                    // CPU Thresholds
                    static const uint32_t numProcessors = Env::GetNumProcessors();
                    buffer.AppendFormat( "CPUs: %u/%u\n", workerSettings.GetNumCPUsToUse(), numProcessors );

                    // Move
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
                    if (fs.Open( m_BrokerageFilePath.Get(), FileStream::WRITE_ONLY ))
                    {
                        fs.WriteBuffer( buffer.Get(), buffer.GetLength() );

                        // Take note of time we wrote the settings
                        m_SettingsWriteTime = settingsWriteTime;
                    }
                }
                m_Availability = available;
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
        m_Availability = available;
    }

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
