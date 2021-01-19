// WorkerThreadRemote - object to process and manage jobs on a remote thread
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "WorkerThread.h"
#include "Core/Process/Mutex.h"

// Forward Declarations
//------------------------------------------------------------------------------
class Job;

// WorkerThread
//------------------------------------------------------------------------------
class WorkerThreadRemote : public WorkerThread
{
public:
    explicit WorkerThreadRemote( uint16_t threadIndex );
    virtual ~WorkerThreadRemote() override;

    void GetStatus( AString & hostName, AString & status, bool & isIdle ) const;

    // control remote CPU usage
    static void     SetNumCPUsToUse( uint32_t c ) { s_NumCPUsToUse = c; }
    static uint32_t GetNumCPUsToUse() { return s_NumCPUsToUse; }
private:
    virtual void Main() override;

    bool IsEnabled() const;

    mutable Mutex m_CurrentJobMutex;
    Job * m_CurrentJob;

    // static
    static uint32_t s_NumCPUsToUse;
};

//------------------------------------------------------------------------------
