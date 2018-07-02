// WorkerThread
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

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
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"

// Static
//------------------------------------------------------------------------------
static THREAD_LOCAL uint32_t s_WorkerThreadThreadIndex = 0;
AStackString<> WorkerThread::s_TmpRoot;

//------------------------------------------------------------------------------
WorkerThread::WorkerThread( uint32_t threadIndex )
: m_ShouldExit( false )
, m_Exited( false )
, m_ThreadIndex( threadIndex )
{
}

// Init
//------------------------------------------------------------------------------
void WorkerThread::Init()
{
    PROFILE_FUNCTION

    // Start thread
    Thread::ThreadHandle h = Thread::CreateThread( ThreadWrapperFunc,
                                                   "WorkerThread",
                                                   64 * KILOBYTE,
                                                   this );
    ASSERT( h != nullptr );
    Thread::DetachThread( h );
    Thread::CloseHandle( h ); // we don't want to keep this, so free it now
}

//------------------------------------------------------------------------------
WorkerThread::~WorkerThread()
{
    ASSERT( m_Exited );
}

// InitTmpDir
//------------------------------------------------------------------------------
/*static*/ void WorkerThread::InitTmpDir(
    const bool sandboxEnabled,
    const AString & obfuscatedSandboxTmp,
    bool remote )
{
    PROFILE_FUNCTION

    if ( sandboxEnabled )
    {
        if ( !obfuscatedSandboxTmp.IsEmpty() )
        {
            s_TmpRoot = obfuscatedSandboxTmp;
            PathUtils::EnsureTrailingSlash( s_TmpRoot );
        }
    }
    else
    {
        VERIFY( FBuild::GetTempDir( s_TmpRoot ) );
    #if defined( __WINDOWS__ )
        s_TmpRoot += ".fbuild.tmp\\";
    #else
        s_TmpRoot += "_fbuild.tmp/";
    #endif
    }

    if ( remote )
    {
        s_TmpRoot += "0";
    }
    else
    {
        const AString & workingDir = FBuild::Get().GetOptions().GetWorkingDir();
        if ( s_TmpRoot.BeginsWith( workingDir ) )
        {
            s_TmpRoot += "0";
        }
        else
        {
            // use the working dir hash to uniquify the path
            AStackString<> buffer;
            const uint32_t workingDirHash = FBuild::Get().GetOptions().GetWorkingDirHash();
            buffer.Format( "0x%08x", workingDirHash );
            s_TmpRoot += buffer;
        }
    }
    s_TmpRoot += NATIVE_SLASH;
    VERIFY( FileIO::EnsurePathExists( s_TmpRoot ) );
}

// WaitForStop
//------------------------------------------------------------------------------
void WorkerThread::WaitForStop()
{
    PROFILE_FUNCTION
    m_MainThreadWaitForExit.Wait();
}

// GetThreadIndex
//------------------------------------------------------------------------------
/*static*/ uint32_t WorkerThread::GetThreadIndex()
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
        threadName.Format( "%s_%u", s_WorkerThreadThreadIndex > 1000 ? "RemoteWorkerThread" : "WorkerThread", s_WorkerThreadThreadIndex );
        PROFILE_SET_THREAD_NAME( threadName.Get() );
    #endif

    CreateThreadLocalTmpDir();

    wt->Main();
    return 0;
}

// Main
//------------------------------------------------------------------------------
/*virtual*/ void WorkerThread::Main()
{
    PROFILE_SECTION( "WorkerThread" )

    for (;;)
    {
        // Wait for work to become available (or quit signal)
        JobQueue::Get().WorkerThreadWait( 500 );

        if ( m_ShouldExit || FBuild::GetStopBuild() )
        {
            break;
        }

        Update();
    }

    m_Exited = true;

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

    // do not filter by local worker tags here,
    // worker filtering is done in the node's DoBuild() instead;
    // since we need to call DoBuild() at least once to queue up 
    // distributable jobs that can work on remote workers
    Job * job = JobQueue::IsValid() ?
        JobQueue::Get().GetJobToProcess() : nullptr;
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
        bool errored = false;
        job = JobQueue::IsValid() ?
            JobQueue::Get().GetDistributableJobToProcess(
                JobQueue::Get().GetLocalWorkerTags(), false, errored ) : nullptr;
        if ( errored )
        {
            // GetDistributableJobToProcess() above will emit error
            FBuild::OnBuildError();
        }
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
    else
    {
        // not running any local jobs
        // check unmatched jobs
        if ( JobQueue::IsValid() )
        {
            bool errored = false;
            JobQueue::Get().CheckUnmatchedJobs( errored );
            if ( errored )
            {
                // CheckUnmatchedJobs() above will emit error
                FBuild::OnBuildError();
            }
        }
    }

    // race remote jobs
    if ( FBuild::Get().GetOptions().m_AllowLocalRace )
    {
        job = JobQueue::IsValid() ?
            JobQueue::Get().GetDistributableJobToRace(
                JobQueue::Get().GetLocalWorkerTags() ) : nullptr;
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
    ASSERT( !s_TmpRoot.IsEmpty() );

    // get the index for the worker thread
    // (for the main thread, this will be 0 which is OK)
    const uint32_t threadIndex = WorkerThread::GetThreadIndex();

    tmpFileDirectory.Format( "%s%s%u", s_TmpRoot.Get(), CORE_PHRASE, threadIndex );
}

// CreateTempFile
//------------------------------------------------------------------------------
/*static*/ void WorkerThread::CreateTempFilePath( const char * fileName,
                                                  AString & tmpFileName )
{
    ASSERT( fileName );

    GetTempFileDirectory( tmpFileName );
    tmpFileName.AppendFormat( "%c%s", NATIVE_SLASH, fileName );
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
    // create isolated subdir
    AStackString<> tmpFileName;
    CreateTempFilePath( ".tmp", tmpFileName );
    char * lastSlash = tmpFileName.FindLast( NATIVE_SLASH );
    tmpFileName.SetLength( (uint32_t)( lastSlash - tmpFileName.Get() ) );
    FileIO::EnsurePathExists( tmpFileName );
}

//------------------------------------------------------------------------------
