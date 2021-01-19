// WorkerThread
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerThread.h"
#include "Job.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Graph/Node.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueue.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

// Core
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"

// Static
//------------------------------------------------------------------------------
static THREAD_LOCAL uint16_t s_WorkerThreadThreadIndex = 0;
Mutex WorkerThread::s_TmpRootMutex;
AStackString<> WorkerThread::s_TmpRoot;

//------------------------------------------------------------------------------
WorkerThread::WorkerThread( uint16_t threadIndex )
: m_ShouldExit( false )
, m_Exited( false )
, m_ThreadIndex( threadIndex )
{
}

// Init
//------------------------------------------------------------------------------
void WorkerThread::Init()
{
    PROFILE_FUNCTION;

    // Start thread
    Thread::ThreadHandle h = Thread::CreateThread( ThreadWrapperFunc,
                                                   "WorkerThread",
                                                   MEGABYTE,
                                                   this );
    ASSERT( h != nullptr );
    Thread::DetachThread( h );
    Thread::CloseHandle( h ); // we don't want to keep this, so free it now
}

//------------------------------------------------------------------------------
WorkerThread::~WorkerThread()
{
    ASSERT( AtomicLoadRelaxed( &m_Exited ) );
}

// InitTmpDir
//------------------------------------------------------------------------------
/*static*/ void WorkerThread::InitTmpDir( bool remote )
{
    PROFILE_FUNCTION;

    AStackString<> tmpDirPath;
    VERIFY( FBuild::GetTempDir( tmpDirPath ) );
    #if defined( __WINDOWS__ )
        tmpDirPath += ".fbuild.tmp\\";
    #else
        tmpDirPath += "_fbuild.tmp/";
    #endif

    // use the working dir hash to uniquify the path
    const uint32_t workingDirHash = remote ? 0 : FBuild::Get().GetOptions().GetWorkingDirHash();
    tmpDirPath.AppendFormat( "0x%08x", workingDirHash );
    tmpDirPath += NATIVE_SLASH;

    VERIFY( FileIO::EnsurePathExists( tmpDirPath ) );

    MutexHolder lock( s_TmpRootMutex );
    s_TmpRoot = tmpDirPath;
}

// Stop
//------------------------------------------------------------------------------
void WorkerThread::Stop()
{
    AtomicStoreRelaxed( &m_ShouldExit, true );
}

// HasExited
//------------------------------------------------------------------------------
bool WorkerThread::HasExited() const
{
    return AtomicLoadRelaxed( &m_Exited );
}

// WaitForStop
//------------------------------------------------------------------------------
void WorkerThread::WaitForStop()
{
    PROFILE_FUNCTION;
    m_MainThreadWaitForExit.Wait();
}

// GetThreadIndex
//------------------------------------------------------------------------------
/*static*/ uint16_t WorkerThread::GetThreadIndex()
{
    return s_WorkerThreadThreadIndex;
}

// MainWrapper
//------------------------------------------------------------------------------
/*static*/ uint32_t WorkerThread::ThreadWrapperFunc( void * param )
{
    WorkerThread * wt = static_cast< WorkerThread * >( param );
    s_WorkerThreadThreadIndex = wt->m_ThreadIndex;

    #if defined( PROFILING_ENABLED )
        AStackString<> threadName;
        threadName.Format( "%s_%02u", s_WorkerThreadThreadIndex > 1000 ? "RemoteWorkerThread" : "WorkerThread", s_WorkerThreadThreadIndex );
        PROFILE_SET_THREAD_NAME( threadName.Get() );
    #endif

    wt->Main();
    return 0;
}

// Main
//------------------------------------------------------------------------------
/*virtual*/ void WorkerThread::Main()
{
    PROFILE_SECTION( "WorkerThread" );

    CreateThreadLocalTmpDir();

    for (;;)
    {
        // Wait for work to become available (or quit signal)
        JobQueue::Get().WorkerThreadWait( 500 );

        if ( AtomicLoadRelaxed( &m_ShouldExit ) || FBuild::GetStopBuild() )
        {
            break;
        }

        Update();
    }

    AtomicStoreRelaxed( &m_Exited, true );

    // wake up main thread
    if ( JobQueue::IsValid() ) // Unit Tests
    {
        JobQueue::Get().WakeMainThread();
    }

    m_MainThreadWaitForExit.Signal();
}

// Update
//------------------------------------------------------------------------------
/*static*/ bool WorkerThread::Update()
{
    // try to find some work to do
    Job * job = JobQueue::IsValid() ? JobQueue::Get().GetJobToProcess() : nullptr;
    if ( job != nullptr )
    {
        // make sure state is as expected
        ASSERT( job->GetNode()->GetState() == Node::BUILDING );

        // process the work
        Node::BuildResult result = JobQueue::DoBuild( job );

        if ( result == Node::NODE_RESULT_FAILED )
        {
            FBuild::OnBuildError();
        }

        if ( result == Node::NODE_RESULT_NEED_SECOND_BUILD_PASS )
        {
            // Only distributable jobs have two passes, and the 2nd pass is always distributable
            JobQueue::Get().QueueDistributableJob( job );
        }
        else
        {
            JobQueue::Get().FinishedProcessingJob( job, ( result != Node::NODE_RESULT_FAILED ), false );
        }

        return true; // did some work
    }

    // no local job, see if we can do one from the remote queue
    if ( FBuild::Get().GetOptions().m_NoLocalConsumptionOfRemoteJobs == false )
    {
        job = JobQueue::IsValid() ? JobQueue::Get().GetDistributableJobToProcess( false ) : nullptr;
        if ( job != nullptr )
        {
            // process the work
            Node::BuildResult result = JobQueueRemote::DoBuild( job, false );

            if ( result == Node::NODE_RESULT_FAILED )
            {
                FBuild::OnBuildError();
            }

            JobQueue::Get().FinishedProcessingJob( job, ( result != Node::NODE_RESULT_FAILED ), true ); // returning a remote job

            return true; // did some work
        }
    }

    // race remote jobs
    if ( FBuild::Get().GetOptions().m_AllowLocalRace )
    {
        job = JobQueue::IsValid() ? JobQueue::Get().GetDistributableJobToRace() : nullptr;
        if ( job != nullptr )
        {
            // process the work
            Node::BuildResult result = JobQueueRemote::DoBuild( job, true );

            if ( result == Node::NODE_RESULT_FAILED )
            {
                // Ignore error if cancelling due to a remote race win
                if ( job->GetDistributionState() != Job::DIST_RACE_WON_REMOTELY_CANCEL_LOCAL )
                {
                    FBuild::OnBuildError();
                }
            }

            JobQueue::Get().FinishedProcessingJob( job, ( result != Node::NODE_RESULT_FAILED ), true ); // returning a remote job

            return true; // did some work
        }
    }

    return false; // no work to do
}


// GetTempFileDirectory
//------------------------------------------------------------------------------
/*static*/ void WorkerThread::GetTempFileDirectory( AString & tmpFileDirectory )
{
    // get the index for the worker thread
    // (for the main thread, this will be 0 which is OK)
    const uint32_t threadIndex = WorkerThread::GetThreadIndex();

    MutexHolder lock( s_TmpRootMutex );
    ASSERT( !s_TmpRoot.IsEmpty() );
    tmpFileDirectory.Format( "%score_%u%c", s_TmpRoot.Get(), threadIndex, NATIVE_SLASH );
}

// CreateTempFile
//------------------------------------------------------------------------------
/*static*/ void WorkerThread::CreateTempFilePath( const char * fileName,
                                                  AString & tmpFileName )
{
    ASSERT( fileName );

    GetTempFileDirectory( tmpFileName );
    tmpFileName += fileName;
}

// CreateTempFile
//------------------------------------------------------------------------------
/*static*/ bool WorkerThread::CreateTempFile( const AString & tmpFileName,
                                        FileStream & file )
{
    ASSERT( tmpFileName.IsEmpty() == false );
    ASSERT( PathUtils::IsFullPath( tmpFileName ) );
    return file.Open( tmpFileName.Get(), FileStream::WRITE_ONLY );
}

// CreateThreadLocalTmpDir
//------------------------------------------------------------------------------
/*static*/ void WorkerThread::CreateThreadLocalTmpDir()
{
    PROFILE_FUNCTION;

    // create isolated subdir
    AStackString<> tmpFileName;
    CreateTempFilePath( ".tmp", tmpFileName );
    const char * lastSlash = tmpFileName.FindLast( NATIVE_SLASH );
    tmpFileName.SetLength( (uint32_t)( lastSlash - tmpFileName.Get() ) );
    FileIO::EnsurePathExists( tmpFileName );
}

//------------------------------------------------------------------------------
