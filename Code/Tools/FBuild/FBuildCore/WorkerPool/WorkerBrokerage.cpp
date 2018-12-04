// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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

// operator == (const BrokerageRecord &)
//------------------------------------------------------------------------------
bool WorkerBrokerage::BrokerageRecord::operator == ( const BrokerageRecord & other ) const
{
    return ( m_DirPath == other.m_DirPath &&
                 m_FilePath == other.m_FilePath );
}

// operator == (const AString &)
//------------------------------------------------------------------------------
bool WorkerBrokerage::TagCache::operator == ( const AString & other ) const
{
    return m_Key == other;  // only compare against key, not tags
}

// operator == (const AString &)
//------------------------------------------------------------------------------
bool WorkerBrokerage::WorkerCache::operator == ( const AString & other ) const
{
    return m_Key == other;  // only compare against key, not workers
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

    // root dir
    AStackString<> root;
    if ( Env::GetEnvVariable( "FASTBUILD_BROKERAGE_PATH", root ) )
    {
        // <path>/<group>/<version>/
        #if defined( __WINDOWS__ )
            m_BrokerageRoot.Format( "%s\\main\\%u", root.Get(), protocolVersion );
        #else
            m_BrokerageRoot.Format( "%s/main/%u", root.Get(), protocolVersion );
        #endif
        m_TagsRoot = m_BrokerageRoot;
        m_TagsRoot += NATIVE_SLASH;
        m_TagsRoot += "tags";
    }

    Network::GetHostName(m_HostName);

    m_TimerLastUpdate.Start();

    m_Initialized = true;
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
    const AString & path, const bool includeDirs,
    Array< AString > & contents ) const
{
    // wildcard search for any worker
    // the filenames are the machine names
    AStackString<> wildcardFileName( "*" );
    Array< AString > pathResults( 256, true );  // start with 256 capacity
    FileIO::GetFiles( path,
                            wildcardFileName,  
                            false,  // recurse
                            includeDirs,
                            &pathResults );
    const size_t numResults = pathResults.GetSize();
    contents.SetCapacity( numResults );
    for ( size_t i=0; i<numResults; ++i )
    {
        const char * lastSlash = pathResults.Get( i ).FindLast( NATIVE_SLASH );
        AStackString<> machineName( lastSlash + 1 );
        contents.Append( machineName );
    }
}

// AddRootWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::AddRootWorkers(
    const Array< AString > & excludedWorkers,
    bool & rootWorkersValid,
    Array< AString > & rootWorkers,
    Array< AString > & rootWorkersToAdd ) const
{
    if ( !rootWorkersValid )
    {
        const bool includeDirs = false;  // just get files ( worker name files )
        ListDirContents( m_BrokerageRoot, includeDirs, rootWorkers );
        rootWorkersValid = true;
    }
    rootWorkersToAdd.SetCapacity( rootWorkers.GetSize() );
    for ( size_t i=0; i<rootWorkers.GetSize(); ++i )
    {
        const AString & rootWorker = rootWorkers.Get( i );
        if ( rootWorker.CompareI( m_HostName ) != 0 &&
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
    const AString & key,
    Array< TagCache > & tagsCache,
    const TagCache * & foundTagCache ) const
{
    foundTagCache = tagsCache.Find( key );
    if ( foundTagCache == nullptr )
    {
        TagCache tagCache;
        tagCache.m_Key = key;
        Array< AString > tagDirs;
        AStackString<> keyDir( m_TagsRoot );
        keyDir += NATIVE_SLASH;
        keyDir += key;
        const bool includeDirs = true;  // include dirs ( value dirs )
        ListDirContents( keyDir, includeDirs, tagDirs );
        for ( size_t i=0; i<tagDirs.GetSize(); ++i )
        {
            const AString & tagDir = tagDirs.Get( i );
            AStackString<> valueToUse;
            if ( tagDir.BeginsWith( TAG_NOT_OPERATOR_DIR_PREFIX ) )
            {
                valueToUse = TAG_NOT_OPERATOR;
                valueToUse += ( tagDir.Get() + strlen( TAG_NOT_OPERATOR_DIR_PREFIX ) );
            }
            else
            {
                valueToUse = tagDir;
            }
            Tag tag;
            tag.SetKey( key );
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
        AStackString<> valueDir( m_TagsRoot );
        for ( size_t i=0; i<dirNameArray.GetSize(); ++i )
        {
            valueDir += NATIVE_SLASH;
            valueDir += dirNameArray.Get( i );
        }
        Array< AString > workerFileNames;
        const bool includeDirs = false;  // just get files ( worker name files )
        ListDirContents( valueDir, includeDirs, workerFileNames );
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
    const Array< AString > & workersToAdd,
    Array< AString > & workersForJob ) const
{
    // calc intersection of workersForJob with workersToAdd
    Array< AString> workersToRemove;
    for ( size_t i=0; i<workersForJob.GetSize(); ++i )
    {
        const AString & jobWorker = workersForJob.Get( i );
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

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerage::FindWorkers(
    const Array< Tags > & requiredWorkerTagsList,
    const Array< AString > & excludedWorkers,
    Array< Array< AString > > & workers )
{
    PROFILE_FUNCTION

    Init();

    if ( m_BrokerageRoot.IsEmpty() )
    {
        FLOG_WARN( "No brokerage root; did you set FASTBUILD_BROKERAGE_PATH?" );
        return;
    }

    bool rootWorkersValid = false;
    bool rootTagDirsValid = false;
    Array< AString > rootWorkers;
    Array< AString > rootTagDirs;
    Array< TagCache > tagsCache;
    Array< WorkerCache > workersCache;
    // loop over each job
    for ( size_t i=0; i<requiredWorkerTagsList.GetSize(); ++i )
    {
        Array< AString > workersForJob;
        const Tags & requiredWorkerTags = requiredWorkerTagsList.Get( i );
        if ( requiredWorkerTags.IsEmpty() )  // job has no required tags
        {
            AddRootWorkers(
                excludedWorkers,
                rootWorkersValid,
                rootWorkers,
                workersForJob );
            workers.Append( workersForJob );
        }
        else  // job has required tags
        {
            // loop over each tag
            for ( size_t j=0; j<requiredWorkerTags.GetSize(); ++j )
            {
                const Tag & requiredWorkerTag = requiredWorkerTags.Get( j );
                // loop over each dir name
                if ( !rootTagDirsValid )
                {
                    const bool includeDirs = true;  // include dirs ( tag dirs )
                    ListDirContents( m_TagsRoot, includeDirs, rootTagDirs );
                    rootTagDirsValid = true;
                }
                bool foundKey = false;
                for ( size_t k=0; k<rootTagDirs.GetSize(); ++k )
                {
                    const AString & keyDirName = rootTagDirs.Get( k );
                    if ( keyDirName.Matches( requiredWorkerTag.GetKey().Get() ) )
                    {
                        foundKey = true;
                        const TagCache * foundTagCache = nullptr;
                        GetTagCache( keyDirName, tagsCache, foundTagCache );
                        if ( foundTagCache != nullptr )
                        {
                            Array< AString> workersToAdd;
                            const Tags & foundTags = foundTagCache->m_Tags;
                            for ( size_t l=0; l<foundTags.GetSize(); ++l )
                            {
                                const Tag & foundTag = foundTags.Get( l );
                                if ( ( requiredWorkerTag.GetKeyInverted() && requiredWorkerTag.GetValue().IsEmpty() ) || 
                                     foundTag.Matches( requiredWorkerTag ) )
                                {
                                    Array< AString > dirNameArray;
                                    foundTag.ToDirNameArray( dirNameArray );
                                    AStackString<> workerCacheKey;
                                    for ( size_t m=0; m<dirNameArray.GetSize(); ++m )
                                    {
                                        workerCacheKey += dirNameArray.Get( m );
                                    }
                                    const WorkerCache * foundWorkerCache = nullptr;
                                    GetWorkersCache( workerCacheKey, dirNameArray, workersCache, foundWorkerCache);
                                    if ( foundWorkerCache != nullptr )
                                    {
                                        const Array< AString > & foundWorkers = foundWorkerCache->m_Workers;
                                        if ( requiredWorkerTag.GetKeyInverted() && requiredWorkerTag.GetValue().IsEmpty() )
                                        {
                                            if ( j == 0 )  // if first required tag for this job
                                            {
                                                // add root workers that are not in the found list ( computes inversion )
                                                Array< AString > rootWorkersToAdd;
                                                AddRootWorkers(
                                                    excludedWorkers,
                                                    rootWorkersValid,
                                                    rootWorkers,
                                                    rootWorkersToAdd );
                                                for ( size_t m=0; m<rootWorkersToAdd.GetSize(); ++m )
                                                {
                                                    const AString & rootWorkerToAdd = rootWorkersToAdd.Get( m );
                                                    if ( !foundWorkers.Find( rootWorkerToAdd ) )
                                                    {
                                                        if ( !workersToAdd.Find( rootWorkerToAdd ) )
                                                        {
                                                            workersToAdd.Append( rootWorkerToAdd );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            // add found workers
                                            for ( size_t m=0; m<foundWorkers.GetSize(); ++m )
                                            {
                                                const AString & foundWorker = foundWorkers.Get( m );
                                                if ( foundWorker.CompareI( m_HostName ) != 0 && 
                                                     !workersToAdd.Find( foundWorker ) && 
                                                     !excludedWorkers.Find( foundWorker ) )
                                                {
                                                    workersToAdd.Append( foundWorker );
                                                }
                                            }
                                        }
                                    }
                                }
                                if ( j == 0 )  // if first required tag for this job
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
                    }
                }
                if ( !foundKey )
                {
                    if ( requiredWorkerTag.GetKeyInverted() && requiredWorkerTag.GetValue().IsEmpty() )
                    {
                        if ( j == 0 )  // if first required tag for this job
                        {
                            // add workers from root workers list
                            AddRootWorkers(
                                excludedWorkers,
                                rootWorkersValid,
                                rootWorkers,
                                workersForJob );
                        }
                        else
                        {
                            Array< AString > rootWorkersToAdd;
                            AddRootWorkers(
                                excludedWorkers,
                                rootWorkersValid,
                                rootWorkers,
                                rootWorkersToAdd );
                            // calc intersection of previous worker list with the root workers to add
                            CalcWorkerIntersection( rootWorkersToAdd, workersForJob );
                        }
                    }
                    else
                    {
                        // no matching key for required tag, so no match for this job
                        workersForJob.Clear();
                    }
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
}

// GetBrokerageRecordsFromTags
//------------------------------------------------------------------------------
void WorkerBrokerage::GetBrokerageRecordsFromTags(
    const Tags & workerTags,
    Array<BrokerageRecord> & brokerageRecords ) const
{
    for ( size_t i=0; i<workerTags.GetSize(); ++i )
    {
        const Tag & workerTag = workerTags.Get( i );
        Array<AString> tagDirNames;
        workerTag.ToDirNameArray( tagDirNames );
        AStackString<> brokerageFileDir( m_TagsRoot );
        const size_t numDirNames = tagDirNames.GetSize();
        if ( numDirNames > 0 )
        {
            for ( size_t j=0; j<numDirNames; ++j )
            {
                brokerageFileDir += NATIVE_SLASH;
                brokerageFileDir += tagDirNames.Get( j );
            }
            // include file path for the worker tag
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
    // include one record for every worker tag
    // will be searched for by client jobs that require tags
    GetBrokerageRecordsFromTags( removedTags, removedBrokerageRecords );
    GetBrokerageRecordsFromTags( addedTags, addedBrokerageRecords);

    // include a record for the brokerage root
    // will be searched for by client jobs that require no tags
    AStackString<> brokerageFilePath( m_BrokerageRoot );
    brokerageFilePath += NATIVE_SLASH;
    brokerageFilePath += m_HostName;
    BrokerageRecord brokerageRecord;
    brokerageRecord.m_DirPath = m_BrokerageRoot;
    brokerageRecord.m_FilePath = brokerageFilePath;
    addedBrokerageRecords.Append( brokerageRecord );
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

// SetAvailable
//------------------------------------------------------------------------------
void WorkerBrokerage::SetAvailable( const Tags & workerTags )
{
    Init();

    // ignore if brokerage not configured
    if ( m_BrokerageRoot.IsEmpty() )
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
            updateFiles = m_TimerLastUpdate.GetElapsedMS() >= 10000.0f;
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
        if ( updateCache )
        {
            Array<BrokerageRecord> removedBrokerageRecords;
            Array<BrokerageRecord> addedBrokerageRecords;
            GetBrokerageRecordChanges(
                removedTags, addedTags,
                removedBrokerageRecords, addedBrokerageRecords );
            // update cache members
            m_WorkerTags.ApplyChanges( removedTags, addedTags );
            ApplyBrokerageRecordChanges( removedBrokerageRecords, addedBrokerageRecords );
            // remove files for removed records
            RemoveBrokerageFiles( removedBrokerageRecords );
        }

        // create files to signify availability
        for ( size_t i=0; i<m_BrokerageRecords.GetSize(); ++i )
        {
            const BrokerageRecord & brokerageRecord = m_BrokerageRecords.Get( i );
            if ( !FileIO::FileExists( brokerageRecord.m_FilePath.Get() ) )
            {
                FileStream fs;
                // create the dir path down to the file
                FileIO::EnsurePathExists( brokerageRecord.m_DirPath );
                // create empty file; clients look at the dir names and filename,
                // not the contents of the file
                fs.Open( brokerageRecord.m_FilePath.Get(), FileStream::WRITE_ONLY );
            }
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
        RemoveBrokerageFiles( m_BrokerageRecords );

        // Restart the timer
        m_TimerLastUpdate.Start();
        m_Availability = false;
    }
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

//------------------------------------------------------------------------------
