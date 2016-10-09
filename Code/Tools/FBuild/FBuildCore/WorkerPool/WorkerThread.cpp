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
    Thread::CloseHandle( h ); // we don't want to keep this, so free it now
}

//------------------------------------------------------------------------------
WorkerThread::~WorkerThread()
{
    ASSERT( m_Exited );
}

// InitTmpDir
//------------------------------------------------------------------------------
/*static*/ void WorkerThread::InitTmpDir( bool remote )
{
    PROFILE_FUNCTION

    VERIFY( FileIO::GetTempDir( s_TmpRoot ) );
    #if defined( __WINDOWS__ )
        s_TmpRoot += ".fbuild.tmp\\";
    #else
        s_TmpRoot += "_fbuild.tmp/";
    #endif

    // use the working dir hash to uniquify the path
    AStackString<> buffer;
    const uint32_t workingDirHash = remote ? 0 : FBuild::Get().GetOptions().GetWorkingDirHash();
    buffer.Format( "0x%08x", workingDirHash );
    s_TmpRoot += buffer;
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
            JobQueue::Get().QueueJob2( job );
        }
        else
        {
            JobQueue::Get().FinishedProcessingJob( job, ( result != Node::NODE_RESULT_FAILED ), false, false );
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

            JobQueue::Get().FinishedProcessingJob( job, ( result != Node::NODE_RESULT_FAILED ), true, false ); // returning a remote job, not a local race

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
                FBuild::OnBuildError();
            }

            JobQueue::Get().FinishedProcessingJob( job, ( result != Node::NODE_RESULT_FAILED ), true, true ); // returning a remote job, local race

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
    // create isolated subdir
    AStackString<> tmpFileName;
    CreateTempFilePath( ".tmp", tmpFileName );
    char * lastSlash = tmpFileName.FindLast( NATIVE_SLASH );
    tmpFileName.SetLength( (uint32_t)( lastSlash - tmpFileName.Get() ) );
    FileIO::EnsurePathExists( tmpFileName );
}

//------------------------------------------------------------------------------
