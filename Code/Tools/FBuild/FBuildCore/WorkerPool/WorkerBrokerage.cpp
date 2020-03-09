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
#include "Core/Env/Assert.h"
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

// operator == (const BrokerageRecord &)
//------------------------------------------------------------------------------
bool WorkerBrokerage::BrokerageRecord::operator == ( const BrokerageRecord & other ) const
{
    // use case insensitive compare, since dir and file names
    return ( m_DirPath.EqualsI( other.m_DirPath ) &&
             m_FilePath.EqualsI( other.m_FilePath ) );
}

// operator == (const AString &)
//------------------------------------------------------------------------------
bool WorkerBrokerage::TagCache::operator == ( const AString & other ) const
{
    // use case insensitive compare, since keys are used as dir names
    return m_Key.EqualsI( other );  // only compare against key, not tags
}

// operator == (const AString &)
//------------------------------------------------------------------------------
bool WorkerBrokerage::WorkerCache::operator == ( const AString & other ) const
{
    // use case insensitive compare, since keys are used as dir names
    return m_Key.EqualsI( other );  // only compare against key, not workers
}

// Init
//------------------------------------------------------------------------------
void WorkerBrokerage::Init( const bool isWorker )
{
    PROFILE_FUNCTION

    if ( m_Status.statusValue != BrokerageUninitialized )
    {
        return;
    }

    // brokerage path includes version to reduce unnecssary comms attempts
    uint32_t protocolVersion = Protocol::PROTOCOL_VERSION;

    Network::GetHostName( m_HostName );

    // root dir
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

    if ( isWorker )
    {
        m_TimerLastUpdate.Start();
        m_TimerLastCleanBroker.Start( sBrokerageElapsedTimeBetweenClean ); // Set timer so we trigger right away

        WorkerInfo workerInfo;
        workerInfo.basePath = "";
        workerInfo.name = m_HostName;

        if ( !m_BrokerageRoots.IsEmpty() )
        {
            // the worker only uses the first path in the roots
            workerInfo.basePath = m_BrokerageRoots[0];
            
            FileIO::EnsurePathExists( workerInfo.basePath );
            if ( !FileIO::DirectoryExists( workerInfo.basePath ) )
            {
                StatusFailure statusFailure;
                statusFailure.workerInfo = workerInfo;
                statusFailure.failureValue = FailureNoBrokerageAccess;
                m_Status.failures.Append( statusFailure );
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
    }
    else  // client
    {
        if ( !m_BrokerageRoots.IsEmpty() )
        {
            for( const AString & root : m_BrokerageRoots )
            {
                WorkerInfo workerInfo;
                workerInfo.basePath = root;
                workerInfo.name = m_HostName;

                FileIO::EnsurePathExists( root );
                if ( !FileIO::DirectoryExists( root ) )
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
            WorkerInfo workerInfo;
            workerInfo.basePath = "";
            workerInfo.name = m_HostName;

            StatusFailure statusFailure;
            statusFailure.workerInfo = workerInfo;
            statusFailure.failureValue = FailureNoBrokeragePath;
            m_Status.failures.Append( statusFailure );
        }
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
    RemoveBrokerageFiles( m_BrokerageRecords );
}

// ListDirContents
//------------------------------------------------------------------------------
void WorkerBrokerage::ListDirContents(
    const Array< AString > & roots,
    const Array< AString > & paths,
    const bool includeDirs,
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
                         includeDirs,
                         &results );
        const size_t numResults = results.GetSize();
        contents.SetCapacity( numResults );
        for ( size_t i=0; i<numResults; ++i )
        {
            const char * lastSlash = results.Get( i ).FindLast( NATIVE_SLASH );
            AStackString<> machineName( lastSlash + 1 );
            WorkerInfo workerInfo;
            workerInfo.basePath = root;
            workerInfo.name = machineName;
            contents.Append( workerInfo );
        }
        index += 1;
    }
}

// AddRootWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::AddRootWorkers(
    const Array< WorkerInfo > & excludedWorkers,
    bool & rootWorkersValid,
    Array< WorkerInfo > & rootWorkers,
    Array< WorkerInfo > & rootWorkersToAdd ) const
{
    if ( !rootWorkersValid )
    {
        const bool includeDirs = false;  // just get files ( worker name files )
        ListDirContents(
            m_BrokerageRoots,  // roots
            m_BrokerageRoots,  // paths: in this case same as roots
            includeDirs,
            rootWorkers );
        rootWorkersValid = true;
    }
    rootWorkersToAdd.SetCapacity( rootWorkers.GetSize() );
    for ( size_t i=0; i<rootWorkers.GetSize(); ++i )
    {
        const WorkerInfo & rootWorker = rootWorkers.Get( i );
        // use case insensitive compare, since hostnames are used as dir names
        if ( rootWorker.name.CompareI( m_HostName ) != 0 &&
              !excludedWorkers.Find( rootWorker ) )
        {
            if ( !rootWorkersToAdd.Find( rootWorker ) )
            {
                rootWorkersToAdd.Append( rootWorker );
            }
        }
    }
}

// GetTagCache
//------------------------------------------------------------------------------
void WorkerBrokerage::GetTagCache(
    const WorkerInfo & key,
    Array< TagCache > & tagsCache,
    const TagCache * & foundTagCache ) const
{
    foundTagCache = tagsCache.Find( key.name );
    if ( foundTagCache == nullptr )
    {
        TagCache tagCache;
        tagCache.m_Key = key.name;
        Array< AString > roots;
        roots.Append( key.basePath );
        Array< AString > keyDirs;
        AStackString<> keyDir( key.basePath );
        keyDir += "tags";
        keyDir += NATIVE_SLASH;
        keyDir += key.name;
        keyDirs.Append( keyDir );
        const bool includeDirs = true;  // include dirs ( value dirs )
        Array< WorkerInfo > tagDirs;
        ListDirContents(
            roots,    // roots
            keyDirs,  // paths
            includeDirs,
            tagDirs );
        for ( size_t i=0; i<tagDirs.GetSize(); ++i )
        {
            const WorkerInfo & tagDir = tagDirs.Get( i );
            AStackString<> valueToUse;
            // use case insensitive compare, since values are used as dir names
            if ( !tagDir.name.EqualsI( TAG_TRUE_VALUE ) )
            {
                // use case insensitive compare, since values are used as dir names
                if ( tagDir.name.BeginsWithI( TAG_NOT_OPERATOR_DIR_PREFIX ) )
                {
                    valueToUse = TAG_NOT_OPERATOR;
                    valueToUse += ( tagDir.name.Get() +
                        AString::StrLen( TAG_NOT_OPERATOR_DIR_PREFIX ) );
                }
                else
                {
                    valueToUse = tagDir.name;
                }
            }
            else
            {
                // use empty valueToUse
            }
            Tag tag;
            tag.SetKey( key.name );
            tag.SetValue( valueToUse );
            tagCache.m_Tags.Append( tag );
        }
        tagsCache.Append( tagCache );
        foundTagCache = tagsCache.end() - 1;
    }
}

// GetWorkersCache
//------------------------------------------------------------------------------
void WorkerBrokerage::GetWorkersCache(
    const AString & key,
    const Array< AString > & dirNameArray,
    Array< WorkerCache > & workersCache,
    const WorkerCache * & foundWorkerCache ) const
{
    foundWorkerCache = workersCache.Find( key );
    if ( foundWorkerCache == nullptr )
    {
        WorkerCache workerCache;
        workerCache.m_Key += key;

        Array< AString > valueDirs;
        for( const AString & root : m_BrokerageRoots )
        {
            AStackString<> valueDir( root );
            valueDir += "tags";
            for ( size_t i=0; i<dirNameArray.GetSize(); ++i )
            {
                valueDir += NATIVE_SLASH;
                valueDir += dirNameArray.Get( i );
            }
            valueDirs.Append( valueDir );
        }

        Array< WorkerInfo > workerFileNames;
        const bool includeDirs = false;  // just get files ( worker name files )
        ListDirContents(
            m_BrokerageRoots,  // roots
            valueDirs,         // paths
            includeDirs,
            workerFileNames );
        for ( size_t i=0; i<workerFileNames.GetSize(); ++i )
        {
            workerCache.m_Workers.Append( workerFileNames.Get( i ) );
        }
        workersCache.Append( workerCache );
        foundWorkerCache = workersCache.end() - 1;
    }
}

// CalcWorkerIntersection
//------------------------------------------------------------------------------
void WorkerBrokerage::CalcWorkerIntersection(
    const Array< WorkerInfo > & workersToAdd,
    Array< WorkerInfo > & workersForJob ) const
{
    // calc intersection of workersForJob with workersToAdd
    Array< WorkerInfo > workersToRemove;
    for ( size_t i=0; i<workersForJob.GetSize(); ++i )
    {
        const WorkerInfo & jobWorker = workersForJob.Get( i );
        if ( !workersToAdd.Find( jobWorker ) )
        {
            workersToRemove.Append( jobWorker );
        }
    }
    for ( size_t i=0; i<workersToRemove.GetSize(); ++i )
    {
        workersForJob.FindAndErase( workersToRemove.Get( i ) );
    }
}

// GetWorkersToConsider
//------------------------------------------------------------------------------
void WorkerBrokerage::GetWorkersToConsider(
    const bool privatePool,
    const bool firstTag,
    const Array< WorkerInfo > & workersForJob,
    const Array< WorkerInfo > & excludedWorkers,
    bool & rootWorkersValid,
    Array< WorkerInfo > & rootWorkers,
    Array< WorkerInfo > & workersToConsider ) const
{
    if ( privatePool )
    {
        if ( firstTag )
        {
            // should never get here, we expect FASTBuild
            // to place the first private pool tag first in this tag list
            // so that we can use the current workersForJob below
            ASSERT( false );
        }
        else
        {
            // use current workersForJob
            // since private pool workers are not listed in root folder
            workersToConsider = workersForJob;
        }
    }
    else if ( !workersForJob.IsEmpty() )
    {
        // performance optimization: use existing list if there is one
        workersToConsider = workersForJob;
    }
    else
    {
        // use the general pool workers
        // they are listed in the root folder
        AddRootWorkers(
            excludedWorkers,
            rootWorkersValid,
            rootWorkers,
            workersToConsider );
    }
}

// HandleFoundKey
//------------------------------------------------------------------------------
void WorkerBrokerage::HandleFoundKey(
    const bool privatePool,
    const bool firstTag,
    const Tag & requiredWorkerTag,
    const Array< WorkerInfo > & excludedWorkers,
    const TagCache * foundTagCache,
    bool & rootWorkersValid,
    Array< WorkerInfo > & rootWorkers,
    Array< WorkerCache > & workersCache,
    Array< WorkerInfo > & workersForJob ) const
{
    Array< WorkerInfo > workersToAdd;
    const Tags & foundTags = foundTagCache->m_Tags;
    for ( size_t i=0; i<foundTags.GetSize(); ++i )
    {
        const Tag & foundTag = foundTags.Get( i );
        if ( ( requiredWorkerTag.GetKeyInverted() &&
               requiredWorkerTag.GetValue().IsEmpty()
             ) || foundTag.Matches( requiredWorkerTag ) )
        {
            Array< AString > dirNameArray;
            foundTag.ToDirNameArray( dirNameArray );
            AStackString<> workerCacheKey;
            for ( size_t j=0; j<dirNameArray.GetSize(); ++j )
            {
                workerCacheKey += dirNameArray.Get( j );
            }
            const WorkerCache * foundWorkerCache = nullptr;
            GetWorkersCache( workerCacheKey, dirNameArray, workersCache, foundWorkerCache);
            if ( foundWorkerCache != nullptr )
            {
                const Array< WorkerInfo > & foundWorkers = foundWorkerCache->m_Workers;
                if ( requiredWorkerTag.GetKeyInverted() && requiredWorkerTag.GetValue().IsEmpty() )
                {
                    Array< WorkerInfo > workersToConsider;
                    GetWorkersToConsider(
                        privatePool,
                        firstTag,
                        workersForJob,
                        excludedWorkers,
                        rootWorkersValid,
                        rootWorkers,
                        workersToConsider );
                    // add root workers that are not in the found list ( computes inversion )
                    for ( size_t j=0; j<workersToConsider.GetSize(); ++j )
                    {
                        const WorkerInfo & rootWorkerToAdd = workersToConsider.Get( j );
                        if ( !foundWorkers.Find( rootWorkerToAdd ) )
                        {
                            if ( !workersToAdd.Find( rootWorkerToAdd ) )
                            {
                                workersToAdd.Append( rootWorkerToAdd );
                            }
                        }
                    }
                }
                else
                {
                    // add found workers
                    for ( size_t j=0; j<foundWorkers.GetSize(); ++j )
                    {
                        const WorkerInfo & foundWorker = foundWorkers.Get( j );
                        // use case insensitive compare, since hostnames are used as dir names
                        if ( foundWorker.name.CompareI( m_HostName ) != 0 && 
                             !workersToAdd.Find( foundWorker ) && 
                             !excludedWorkers.Find( foundWorker ) )
                        {
                            workersToAdd.Append( foundWorker );
                        }
                    }
                }
            }
        }
        if ( firstTag )
        {
            // add workers for job
            workersForJob.Append( workersToAdd );
        }
        else  // if subsequent required tag for this job
        {
            // calc intersection of previous worker list with the worker list for this tag
            CalcWorkerIntersection( workersToAdd, workersForJob );
        }
    }
}

// HandleNotFoundKey
//------------------------------------------------------------------------------
void WorkerBrokerage::HandleNotFoundKey(
    const bool privatePool,
    const bool firstTag,
    const Tag & requiredWorkerTag,
    const Array< WorkerInfo > & excludedWorkers,
    bool & rootWorkersValid,
    Array< WorkerInfo > & rootWorkers,
    Array< WorkerInfo > & workersForJob ) const
{
    if ( requiredWorkerTag.GetKeyInverted() &&
         requiredWorkerTag.GetValue().IsEmpty() )
    {
        Array< WorkerInfo > workersToConsider;
        GetWorkersToConsider(
            privatePool,
            firstTag,
            workersForJob,
            excludedWorkers,
            rootWorkersValid,
            rootWorkers,
            workersToConsider );
        if ( firstTag )
        {
            workersForJob = workersToConsider;
        }
        else
        {
            // calc intersection of previous worker list with the workers to consider
            CalcWorkerIntersection( workersToConsider, workersForJob );
        }
    }
    else
    {
        // no matching key for required tag, so no match for this job
        workersForJob.Clear();
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
                default:
                    ASSERT( false ); // should never get here
            }
        }
    }
}

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers(
    const Array< Tags > & requiredWorkerTagsList,
    const Array< WorkerInfo > & excludedWorkers,
    Array< Array< WorkerInfo > > & workers )
{
    PROFILE_FUNCTION

    Init( false );  // isWorker == false

    if ( m_Status.statusValue != WorkerBrokerage::BrokerageSuccess )
    {
        return;
    }

    bool rootWorkersValid = false;
    bool rootTagDirsValid = false;
    Array< WorkerInfo > rootWorkers;
    Array< WorkerInfo > rootTagDirs;
    Array< TagCache > tagsCache;
    Array< WorkerCache > workersCache;
    // loop over each job
    for ( size_t i=0; i<requiredWorkerTagsList.GetSize(); ++i )
    {
        Array< WorkerInfo > workersForJob;
        const Tags & jobRequiredTags = requiredWorkerTagsList.Get( i );
        Tags requiredWorkerTags;
        bool privatePool = false;
        for ( size_t j=0; j<jobRequiredTags.GetSize(); ++j )
        {
            const Tag & jobRequiredTag = jobRequiredTags.Get( j );
            if ( jobRequiredTag.IsPrivatePoolTag() )
            {
                // place the first private pool tag first in the list;
                // this is so we can use the list as the base list for
                // later calculations, instead of the root folder list.
                // We can't use the root folder list because private pool
                // workers are not listed in it.
                requiredWorkerTags.Append( jobRequiredTag );
                privatePool = true;
                break;
            }
        }
        if ( privatePool )
        {
            // add remaining tags
            for ( size_t j=0; j<jobRequiredTags.GetSize(); ++j )
            {
                const Tag & jobRequiredTag = jobRequiredTags.Get( j );
                // if not the first one we added above
                if ( jobRequiredTag != requiredWorkerTags.Get( 0 ) )
                {
                    requiredWorkerTags.Append( jobRequiredTag );
                }
            }
        }
        else
        {
            // use the job's tag order
            requiredWorkerTags = jobRequiredTags;
            // skip workers that have private pool tags
            AStackString<> notPrivatePoolKey( TAG_NOT_OPERATOR );
            notPrivatePoolKey += TAG_PRIVATE_POOL_PREFIX;
            notPrivatePoolKey += "*";
            Tag tag;
            tag.SetKey( notPrivatePoolKey );
            requiredWorkerTags.Append( tag );
        }

        // loop over each tag
        for ( size_t j=0; j<requiredWorkerTags.GetSize(); ++j )
        {
            const bool firstTag = j == 0;  // if first required tag for this job
            const Tag & requiredWorkerTag = requiredWorkerTags.Get( j );
            // loop over each dir name
            if ( !rootTagDirsValid )
            {
                Array< AString > tagRoots( 256, true );
                for( const AString & root : m_BrokerageRoots )
                {
                    AStackString<> tagRoot( root );
                    tagRoot += "tags";
                    tagRoots.Append( tagRoot );
                }
                const bool includeDirs = true;  // include dirs ( tag dirs )
                ListDirContents(
                    m_BrokerageRoots,  // roots
                    tagRoots,          // paths
                    includeDirs,
                    rootTagDirs );
                rootTagDirsValid = true;
            }
            bool foundKey = false;
            for ( size_t k=0; k<rootTagDirs.GetSize(); ++k )
            {
                const WorkerInfo & keyDir = rootTagDirs.Get( k );
                // use case insensitive compare, since keys are used as dir names
                if ( keyDir.name.MatchesI( requiredWorkerTag.GetKey().Get() ) )
                {
                    foundKey = true;
                    const TagCache * foundTagCache = nullptr;
                    GetTagCache( keyDir, tagsCache, foundTagCache );
                    if ( foundTagCache != nullptr )
                    {
                        HandleFoundKey(
                            privatePool,
                            firstTag,
                            requiredWorkerTag,
                            excludedWorkers,
                            foundTagCache,
                            rootWorkersValid,
                            rootWorkers,
                            workersCache,
                            workersForJob );
                    }
                }
            }
            if ( !foundKey )
            {
                HandleNotFoundKey(
                    privatePool,
                    firstTag,
                    requiredWorkerTag,
                    excludedWorkers,
                    rootWorkersValid,
                    rootWorkers,
                    workersForJob );
            }
            if ( workersForJob.IsEmpty() )
            {
                // no workers match this required tag, so break out of checks for this job
                break;
            }
        }
        workers.Append( workersForJob );
    }
}

// GetBrokerageRecordsFromTags
//------------------------------------------------------------------------------
void WorkerBrokerage::GetBrokerageRecordsFromTags(
    const Tags & workerTags,
    Array<BrokerageRecord> & brokerageRecords ) const
{
    // the worker only uses the first path in the roots
    AStackString<> tagsDir( m_BrokerageRoots[0] );
    tagsDir += "tags";

    for ( size_t i=0; i<workerTags.GetSize(); ++i )
    {
        const Tag & workerTag = workerTags.Get( i );
        Array<AString> tagDirNames;
        workerTag.ToDirNameArray( tagDirNames );
        AStackString<> brokerageFileDir( tagsDir );
        const size_t numDirNames = tagDirNames.GetSize();
        if ( numDirNames > 0 )
        {
            for ( size_t j=0; j<numDirNames; ++j )
            {
                brokerageFileDir += NATIVE_SLASH;
                brokerageFileDir += tagDirNames.Get( j );
            }
            // include one record for every worker tag
            // will be searched for by client jobs that require tags
            AStackString<> brokerageFilePath( brokerageFileDir );
            brokerageFilePath += NATIVE_SLASH;
            brokerageFilePath += m_HostName;
            BrokerageRecord brokerageRecord;
            brokerageRecord.m_DirPath = brokerageFileDir;
            brokerageRecord.m_FilePath = brokerageFilePath;
            brokerageRecords.Append( brokerageRecord );
        }
    }
}

// GetBrokerageRecordChanges
//------------------------------------------------------------------------------
void WorkerBrokerage::GetBrokerageRecordChanges(
    const Tags & removedTags,
    const Tags & addedTags,
    Array<BrokerageRecord> & removedBrokerageRecords,
    Array<BrokerageRecord> & addedBrokerageRecords ) const
{
    GetBrokerageRecordsFromTags( removedTags, removedBrokerageRecords );
    GetBrokerageRecordsFromTags( addedTags, addedBrokerageRecords);
}

// ApplyBrokerageRecordChanges
//------------------------------------------------------------------------------
bool WorkerBrokerage::ApplyBrokerageRecordChanges(
    const Array<BrokerageRecord> & removedBrokerageRecords,
    const Array<BrokerageRecord> & addedBrokerageRecords )
{
    bool anyChangesApplied = false;

    // remove records
    const size_t numRemovedRecords = removedBrokerageRecords.GetSize();
    for ( size_t i=0; i<numRemovedRecords; ++i )
    {
        const BrokerageRecord & removedRecord = removedBrokerageRecords.Get( i );
        if ( m_BrokerageRecords.FindAndErase( removedRecord ) )
        {
            anyChangesApplied = true;
        }
    }

    // add records
    const size_t numAddedRecords = addedBrokerageRecords.GetSize();
    for ( size_t i=0; i<numAddedRecords; ++i )
    {
        const BrokerageRecord & addedRecord = addedBrokerageRecords.Get( i );        
        BrokerageRecord * const foundRecord = m_BrokerageRecords.Find( addedRecord );
        if ( !foundRecord )
        {
            m_BrokerageRecords.Append( addedRecord );
            anyChangesApplied = true;
        }
    }

    return anyChangesApplied;
}

// ShouldUpdateAvailability
//------------------------------------------------------------------------------
bool WorkerBrokerage::ShouldUpdateAvailability() const
{
    bool shouldUpdateAvailability = false;

    // update availability if BrokerageSuccess
    if ( m_Status.statusValue == WorkerBrokerage::BrokerageSuccess )
    {
        shouldUpdateAvailability = true;
    }
    return shouldUpdateAvailability;
}

// SetAvailable
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailable(
    const Tags & workerTags )
{
    Init( true );  // isWorker == true

    if ( !ShouldUpdateAvailability() )
    {
        return;
    }

    bool updateFiles = true;
    bool updateCache = true;
    Tags removedTags;
    Tags addedTags;
    if ( m_Availability )
    {
        // if worker tags changed, we will update the cache
        m_WorkerTags.GetChanges( workerTags, removedTags, addedTags );
        updateCache = !removedTags.IsEmpty() || !addedTags.IsEmpty();
        // if update cache, then also update files
        updateFiles = updateCache;
        if ( !updateFiles )
        {
            // fall back to timer
            // check timer, to ensure that the files will be recreated if cleanup is done on the brokerage path
            updateFiles = m_TimerLastUpdate.GetElapsedMS() >= 60000.0f;
            // don't touch updateCache here
        }
        else
        {
            // update bools are true here, so just continue
        }
    }
    else  // not available
    {
        // so we can write the files immediately on worker launch, update all
        addedTags = workerTags;
        // update bools are true here, so just continue
    }

    if ( updateFiles )
    {
        // the worker only uses the first path in the roots
        WorkerInfo workerInfo;
        workerInfo.basePath = m_BrokerageRoots[0];
        workerInfo.name = m_HostName;

        if ( updateCache )
        {
            Array<BrokerageRecord> removedBrokerageRecords;
            Array<BrokerageRecord> addedBrokerageRecords;
            GetBrokerageRecordChanges(
                removedTags, addedTags,
                removedBrokerageRecords, addedBrokerageRecords );
            // update cache members
            m_WorkerTags.ApplyChanges( removedTags, addedTags );

            AStackString<> brokerageFilePath( workerInfo.basePath );
            brokerageFilePath += NATIVE_SLASH;
            brokerageFilePath += m_HostName;
            BrokerageRecord rootBrokerageRecord;
            rootBrokerageRecord.m_DirPath = workerInfo.basePath;
            rootBrokerageRecord.m_FilePath = brokerageFilePath;
            if ( m_WorkerTags.ContainsPrivatePoolTag() )
            {
                // private pool worker, so don't include worker in root folder
                removedBrokerageRecords.Append( rootBrokerageRecord );
            }
            else
            {
                // include worker in the root folder
                // it will be searched for by client jobs that
                // do not require private pool tags
                addedBrokerageRecords.Append( rootBrokerageRecord );
            }

            ApplyBrokerageRecordChanges( removedBrokerageRecords, addedBrokerageRecords );
            // remove files for removed records
            RemoveBrokerageFiles( removedBrokerageRecords );
        }

        m_FailingBrokerageFiles.Clear();
        for ( size_t i=0; i<m_BrokerageRecords.GetSize(); ++i )
        {
            const BrokerageRecord & brokerageRecord = m_BrokerageRecords.Get( i );

            // If settings have changed, (re)create the file 
            // If settings have not changed, update the modification timestamp
            const WorkerSettings & workerSettings = WorkerSettings::Get();
            const uint64_t settingsWriteTime = workerSettings.GetSettingsWriteTime();
            bool createBrokerageFile = ( settingsWriteTime > m_SettingsWriteTime );

            if ( createBrokerageFile == false )
            {
                // Update the modified time
                // (Allows an external process to delete orphaned files (from crashes/terminated workers)
                if ( FileIO::SetFileLastWriteTimeToNow( brokerageRecord.m_FilePath ) == false )
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
                FileIO::EnsurePathExists( workerInfo.basePath );
                FileStream fs;
                fs.Open( brokerageRecord.m_FilePath.Get(), FileStream::WRITE_ONLY );
                fs.WriteBuffer( buffer.Get(), buffer.GetLength() );

                // Take note of time we wrote the settings
                m_SettingsWriteTime = settingsWriteTime;
            }
        }

        // if we got here then we are no longer in the failure
        // case, so clear failures
        m_Status.failures.Clear();
        m_Availability = true;

        // Restart the timer
        m_TimerLastUpdate.Start();

        if ( m_Status.failures.IsEmpty() )
        {
            m_Status.statusValue = BrokerageSuccess;
        }
        else
        {
            m_Status.statusValue = BrokerageFailure;
        }
    }

    PeriodicCleanup();
}

// SetUnavailable
//------------------------------------------------------------------------------
void WorkerBrokerage::SetUnavailable()
{
    Init( true );  // isWorker == true

    if ( !ShouldUpdateAvailability() )
    {
        return;
    }

    // if already available, then make unavailable
    if ( m_Availability )
    {
        // to remove availability, remove the files
        RemoveBrokerageFiles( m_BrokerageRecords );

        // Restart the timer
        m_TimerLastUpdate.Start();
        m_Availability = false;
    }

    PeriodicCleanup();
}

// RemoveBrokerageFiles
//------------------------------------------------------------------------------
/*static*/ void WorkerBrokerage::RemoveBrokerageFiles(
    const Array<BrokerageRecord> & brokerageRecords )
{
    // Ensure the files disappear
    const size_t numBrokerageRecords = brokerageRecords.GetSize();
    if ( numBrokerageRecords > 0 )
    {
        for ( size_t i=0; i<numBrokerageRecords; ++i )
        {
            const BrokerageRecord & brokerageRecord = brokerageRecords.Get( i );
            // only remove the files here
            FileIO::FileDelete( brokerageRecord.m_FilePath.Get() );
            // can't remove the dirs, since doing so would race with the dir creation logic of other workers
        }
    }
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
                                true,   // recursive so we cleanup tag and blacklist files
                                false,  // includeDirs
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
