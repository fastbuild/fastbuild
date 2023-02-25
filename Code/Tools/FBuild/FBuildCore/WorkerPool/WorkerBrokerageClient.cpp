// WorkerBrokerageClient - Client-side worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerageClient.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerageClient::WorkerBrokerageClient() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerageClient::~WorkerBrokerageClient() = default;

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerageClient::FindWorkers( Array< AString > & outWorkerList )
{
    PROFILE_FUNCTION;

    // Check for workers for the FASTBUILD_WORKERS environment variable
    // which is a list of worker addresses separated by a semi-colon.
    AStackString<> workersEnv;
    if ( Env::GetEnvVariable( "FASTBUILD_WORKERS", workersEnv ) )
    {
        // If we find a valid list of workers, we'll use that
        workersEnv.Tokenize( outWorkerList, ';' );
        if ( outWorkerList.IsEmpty() == false )
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
        const size_t filesBeforeSearch = results.GetSize();
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
    if ( ( outWorkerList.GetSize() + results.GetSize() ) > outWorkerList.GetCapacity() )
    {
        outWorkerList.SetCapacity( outWorkerList.GetSize() + results.GetSize() );
    }

    // Get addresses for the local host
    StackArray<AString> localAddresses;
    Network::GetIPv4Addresses( localAddresses );

    // convert worker strings
    for (const AString & fileName : results )
    {
        const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
        AStackString<> workerName( lastSlash + 1 );

        // Filter out local addresses
        if ( localAddresses.Find( workerName ) )
        {
            continue;
        }

        outWorkerList.Append( workerName );
    }
}

//------------------------------------------------------------------------------
