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
    , m_BrokerageStatus( BrokerageUninitialized )
{
}

// Init
//------------------------------------------------------------------------------
void WorkerBrokerage::Init()
{
    PROFILE_FUNCTION

    ASSERT( Thread::IsMainThread() );

    if ( m_BrokerageStatus != BrokerageUninitialized )
    {
        return;
    }

    // brokerage path includes version to reduce unnecssary comms attempts
    uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

    Network::GetHostName(m_HostName);

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
        m_BlacklistDir = m_BrokerageRoot;
        m_BlacklistDir += "blacklist";

        AStackString<> blacklistFile;
        if ( GetBlacklistFile( m_HostName, blacklistFile ) &&
             FileIO::FileExists( blacklistFile.Get() ) )
        {
            m_BrokerageStatus = BlacklistedWorker;
            return;
        }
    }

    AStackString<> filePath;
    m_BrokerageFilePath.Format( "%s%s", m_BrokerageRoot.Get(), m_HostName.Get() );
    m_TimerLastUpdate.Start();

    m_BrokerageStatus = BrokerageSuccess;
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

// GetStatusMsg
//------------------------------------------------------------------------------
void WorkerBrokerage::GetStatusMsg(
    const uint32_t brokerageStatus,
    AString & statusMsg ) const
{
    switch( brokerageStatus )
    {
        case BrokerageSuccess:
            // success case, so no error message to set
            break;
        case BlacklistedWorker:
            {
                statusMsg = "Your worker has been blacklisted.";
                statusMsg += " Please use a different brokerage path and debug your machine issue.";
                AStackString<> blacklistFile;
                if ( GetBlacklistFile( m_HostName, blacklistFile ) )
                {
                    statusMsg += " For issue details, see contents of ";
                    statusMsg += blacklistFile;
                }
            }
            break;
        default:
            ASSERT( false ); // should never get here
    }
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers( Array< AString > & workerList )
{
    PROFILE_FUNCTION

    Init();

    if ( m_BrokerageStatus != WorkerBrokerage::BrokerageSuccess )
    {
        return;
    }

    if ( m_BrokerageRoot.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?" );
        return;
    }

    Array< AString > results;
    ListDirContents( m_BrokerageRoot, results );
    if ( results.IsEmpty() )
    {
        FLOG_WARN( "No workers found in '%s'", m_BrokerageRoot.Get() );
        return; // no files found
    }

    // presize
    if ( ( workerList.GetSize() + results.GetSize() ) > workerList.GetCapacity() )
    {
        workerList.SetCapacity( workerList.GetSize() + results.GetSize() );
    }

    RefreshBlacklistedWorkers();

    const AString * const end = results.End();
    for ( AString * it = results.Begin(); it != end; ++it )
    {
        const AString & workerName = *it;
        if ( workerName.CompareI( m_HostName ) != 0 &&
             !m_BlacklistedWorkers.Find( workerName ) )
        {
            workerList.Append( workerName );
        }
    }
}

// RefreshBlacklistedWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::RefreshBlacklistedWorkers()
{
    if ( !m_BlacklistDir.IsEmpty() )
    {
        m_BlacklistedWorkers.Clear();
        ListDirContents( m_BlacklistDir, m_BlacklistedWorkers );
    }
}

// BlacklistWorker
//------------------------------------------------------------------------------
void WorkerBrokerage::BlacklistWorker(
    const AString & workerName,
    const AString & blacklistReason,
    AString & errorMsg )
{
    if ( m_BrokerageStatus != WorkerBrokerage::BrokerageSuccess )
    {
        return;
    }

    if ( !m_BlacklistedWorkers.Find( workerName ) )
    {
        m_BlacklistedWorkers.Append( workerName );
    }

    if ( !m_BlacklistDir.IsEmpty() )
    {
        AStackString<> fileContents( "Client Machine: " );
        fileContents += m_HostName;
        fileContents += "\nClient User: ";
        if ( m_UserName.IsEmpty() )
        {
            Env::GetLocalUserName( m_UserName );
        }
        fileContents += m_UserName;
        fileContents += "\n";
        fileContents += blacklistReason;

        AStackString<> blacklistFile;
        if ( GetBlacklistFile( workerName, blacklistFile ) &&
             !FileIO::FileExists( blacklistFile.Get() ) )
        {
            FileStream fs;
            // create the dir path down to the file
            FileIO::EnsurePathExists( m_BlacklistDir );
            if ( fs.Open( blacklistFile.Get(), FileStream::WRITE_ONLY ) == false )
            {
                errorMsg.Format( "Failed to open blacklist file %s for writing", blacklistFile.Get() );
                return;
            }
            if ( fs.Write( fileContents.Get(), fileContents.GetLength() ) != fileContents.GetLength() )
            {
                errorMsg.Format( "Failed to write to blacklist file %s", blacklistFile.Get() );
                return;
            }
            fs.Close();
        }
    }
}

// GetBlacklistFile
//------------------------------------------------------------------------------
bool WorkerBrokerage::GetBlacklistFile(
    const AString & workerName,
    AString & filePath ) const
{
    if ( m_BlacklistDir.IsEmpty() )
    {
        return false;
    }
    
    filePath = m_BlacklistDir;
    filePath += NATIVE_SLASH;
    filePath += workerName;
    return true;
}

// SetAvailability
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailability(bool available)
{
    Init();

    // update availability if BrokerageSuccess or BlacklistedWorker
    if ( m_BrokerageStatus != WorkerBrokerage::BrokerageSuccess &&
         m_BrokerageStatus != WorkerBrokerage::BlacklistedWorker )
    {
        return;
    }

    // ignore if brokerage not configured
    if ( m_BrokerageRoot.IsEmpty() )
    {
        return;
    }

    if ( available )
    {
        // Check the last update time to avoid too much File IO.
        float elapsedTime = m_TimerLastUpdate.GetElapsedMS();
        if ( elapsedTime >= 10000.0f )
        {
            AStackString<> blacklistFile;
            if ( GetBlacklistFile( m_HostName, blacklistFile ) &&
                 FileIO::FileExists( blacklistFile.Get() ) )
            {
                m_BrokerageStatus = WorkerBrokerage::BlacklistedWorker;
            }
            else
            {
                //
                // Ensure that the file will be recreated if cleanup is done on the brokerage path.
                //
                if ( !FileIO::FileExists( m_BrokerageFilePath.Get() ) )
                {
                    FileIO::EnsurePathExists( m_BrokerageRoot );

                    // create file to signify availability
                    FileStream fs;
                    fs.Open( m_BrokerageFilePath.Get(), FileStream::WRITE_ONLY );

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
}

//------------------------------------------------------------------------------
