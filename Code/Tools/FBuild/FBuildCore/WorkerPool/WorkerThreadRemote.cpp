// WorkerThreadRemote
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerThreadRemote.h"
#include "Job.h"

#include "Tools/FBuild/FBuildWorker/Worker/WorkerSettings.h"

#include "Tools/FBuild/FBuildCore/FBuild.h"
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Protocol/Server.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/JobQueueRemote.h"

#include "Core/Process/Atomic.h"
#include "Core/Process/Thread.h"
#include "Core/Time/Timer.h"

// Static
//------------------------------------------------------------------------------
/*static*/ uint32_t WorkerThreadRemote::s_NumCPUsToUse( 999 ); // no limit

//------------------------------------------------------------------------------
WorkerThreadRemote::WorkerThreadRemote( uint16_t threadIndex )
: WorkerThread( threadIndex )
, m_CurrentJob( nullptr )
{
}

//------------------------------------------------------------------------------
WorkerThreadRemote::~WorkerThreadRemote()
{
    ASSERT( m_Exited );
}

// Main
//------------------------------------------------------------------------------
/*virtual*/ void WorkerThreadRemote::Main()
{
    while ( AtomicLoadRelaxed( &m_ShouldExit ) == false )
    {
        if ( IsEnabled() == false )
        {
            JobQueueRemote::Get().WorkerThreadSleep();
            continue; // after sleep, check exit condition
        }

        // try to find some work to do
        Job * job = JobQueueRemote::Get().GetJobToProcess();
        if ( job != nullptr )
        {
            {
                MutexHolder mh( m_CurrentJobMutex );
                m_CurrentJob = job;
            }

            // process the work
            Node::BuildResult result = JobQueueRemote::DoBuild( job, false );
            ASSERT( ( result == Node::NODE_RESULT_OK ) || ( result == Node::NODE_RESULT_FAILED ) );

            {
                MutexHolder mh( m_CurrentJobMutex );
                m_CurrentJob = nullptr;
            }

            // Take note of the thread used to build the job
            job->SetRemoteThreadIndex( WorkerThread::GetThreadIndex() );

            JobQueueRemote::Get().FinishedProcessingJob( job, ( result != Node::NODE_RESULT_FAILED ) );

            // loop again to get another job
            continue;
        }
    }

    AtomicStoreRelaxed( &m_Exited, true );

    m_MainThreadWaitForExit.Signal();
}

// GetStatus
//------------------------------------------------------------------------------
void WorkerThreadRemote::GetStatus( AString & hostName, AString & status, bool & isIdle ) const
{
    isIdle = false;

    MutexHolder mh( m_CurrentJobMutex );
    if ( m_CurrentJob )
    {
        Server::GetHostForJob( m_CurrentJob, hostName );
        if ( IsEnabled() == false )
        {
            status = "(Finishing) ";
        }
        status += m_CurrentJob->GetRemoteName();
    }
    else
    {
        hostName.Clear();

        if ( IsEnabled() == false )
        {
            status = "(Disabled)";
        }
        else
        {
            status = "Idle";
            isIdle = true;
        }
    }
}

// IsEnabled
//------------------------------------------------------------------------------
bool WorkerThreadRemote::IsEnabled() const
{
    // determine 1-base CPU identifier
    const uint16_t cpuId = ( m_ThreadIndex - 1000u ); // remote thread index starts at 1001

    // enabled?
    return ( cpuId <= s_NumCPUsToUse );
}

//------------------------------------------------------------------------------
