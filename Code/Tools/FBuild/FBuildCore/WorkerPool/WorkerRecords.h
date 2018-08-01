// WorkerRecords.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

#define LOCAL_WORKER_NAME "localhost"

// WorkerRecord
//------------------------------------------------------------------------------
struct WorkerRecord
{
    public:
        inline explicit WorkerRecord( const AString & workerName, const bool canBuildJob );
        inline virtual ~WorkerRecord() = default;

        inline const AString & GetWorkerName() const;
        inline bool GetCanBuildJob() const;
        inline void SetCanBuildJob( const bool canBuildJob );

    private:
        AString m_WorkerName;
        bool    m_CanBuildJob;
};

// WorkerRecords
//------------------------------------------------------------------------------
class WorkerRecords
{
    public:
        inline WorkerRecords();
        inline virtual ~WorkerRecords() = default;

        inline size_t GetNumLocalWorkers() const;
        inline size_t GetNumRemoteWorkers() const;
        inline size_t GetSize() const;
        inline WorkerRecord & Get( size_t index );
        inline const WorkerRecord & Get( size_t index ) const;
        inline bool Find( const AString& workerName, size_t & foundIndex ) const;
        inline void Update( const AString & workerName, const bool canBuildJob );
        inline void Remove( const AString & workerName );
        inline bool CanBuildJob( const size_t index, const bool includeLocal ) const;
        inline bool CanBuildJob( const bool includeLocal ) const;

    private:
        Array< WorkerRecord > m_WorkerRecords;
        size_t m_NumLocalWorkers;
        size_t m_NumRemoteWorkers;
};

// CONSTRUCTOR (WorkerRecord)
//------------------------------------------------------------------------------
WorkerRecord::WorkerRecord( const AString & workerName, const bool canBuildJob ) :
      m_WorkerName( workerName )
    , m_CanBuildJob( canBuildJob )
{
}

// GetWorkerName (WorkerRecord)
//------------------------------------------------------------------------------
const AString & WorkerRecord::GetWorkerName() const
{
    return m_WorkerName;
}

// GetCanBuildJob (WorkerRecord)
//------------------------------------------------------------------------------
bool WorkerRecord::GetCanBuildJob() const
{
    return m_CanBuildJob;
}

// SetCanBuildJob (WorkerRecord)
//------------------------------------------------------------------------------
void WorkerRecord::SetCanBuildJob( const bool canBuildJob )
{
    m_CanBuildJob = canBuildJob;
}

// CONSTRUCTOR (WorkerRecords)
//------------------------------------------------------------------------------
WorkerRecords::WorkerRecords() :
      m_NumLocalWorkers( 0 )
    , m_NumRemoteWorkers( 0 )
{
}

// GetNumLocalWorkers (WorkerRecords)
//------------------------------------------------------------------------------
size_t WorkerRecords::GetNumLocalWorkers() const
{
    return m_NumLocalWorkers;
}

// GetNumRemoteWorkers (WorkerRecords)
//------------------------------------------------------------------------------
size_t WorkerRecords::GetNumRemoteWorkers() const
{
    return m_NumRemoteWorkers;
}

// GetSize (WorkerRecords)
//------------------------------------------------------------------------------
size_t WorkerRecords::GetSize() const
{
    return m_WorkerRecords.GetSize();
}

// Get non-const (WorkerRecords)
//------------------------------------------------------------------------------
WorkerRecord & WorkerRecords::Get( size_t index )
{
    return m_WorkerRecords[ index ];
}

// Get const (WorkerRecords)
//------------------------------------------------------------------------------
const WorkerRecord & WorkerRecords::Get( size_t index ) const
{
    return m_WorkerRecords[ index ];
}

// Find (WorkerRecords)
//------------------------------------------------------------------------------
bool WorkerRecords::Find( const AString & workerName, size_t & foundIndex ) const
{
    bool foundName = false;
    const size_t numExistingRecords = GetSize();
    for ( size_t i=0; i<numExistingRecords; ++i )
    {
        const WorkerRecord & workerRecord = Get( i );
        if ( workerRecord.GetWorkerName() == workerName )
        {
            foundName = true;
            foundIndex = i;
            break;
        }
    }
    return foundName;
}

// Update (WorkerRecords)
//------------------------------------------------------------------------------
void WorkerRecords::Update( const AString & workerName, const bool canBuildJob )
{
    if ( !workerName.IsEmpty() )
    {
        bool foundName = false;
        const size_t numExistingRecords = GetSize();
        for ( size_t i=0; i<numExistingRecords; ++i )
        {
            WorkerRecord & existingRecord = Get( i );
            if ( existingRecord.GetWorkerName() == workerName )
            {
                foundName = true;
                existingRecord.SetCanBuildJob( canBuildJob );
                break;
            }
        }
        if ( !foundName )
        {
            m_WorkerRecords.Append( WorkerRecord( workerName, canBuildJob ) );
            if ( workerName == LOCAL_WORKER_NAME )
            {
                ++m_NumLocalWorkers;
            }
            else
            {
                ++m_NumRemoteWorkers;
            }
        }
    }
}

// Remove (WorkerRecords)
//------------------------------------------------------------------------------
void WorkerRecords::Remove( const AString & workerName )
{
    if ( !workerName.IsEmpty() )
    {
        const size_t numExistingRecords = GetSize();
        for ( size_t i=0; i<numExistingRecords; ++i )
        {
            const WorkerRecord & existingRecord = Get( i );
            if ( existingRecord.GetWorkerName() == workerName )
            {
                if ( workerName == LOCAL_WORKER_NAME )
                {
                    --m_NumLocalWorkers;
                }
                else
                {
                    --m_NumRemoteWorkers;
                }
                m_WorkerRecords.EraseIndex( i );
                break;
            }
        }
    }
}

// CanBuildJob index (WorkerRecords)
//------------------------------------------------------------------------------
bool WorkerRecords::CanBuildJob( const size_t index, const bool includeLocal ) const
{
    bool canBuildJob = false;
    if ( index < GetSize() )
    {
        const WorkerRecord & existingRecord = Get( index );
        if ( existingRecord.GetCanBuildJob() )
        {
            canBuildJob = includeLocal ||
                ( existingRecord.GetWorkerName() != LOCAL_WORKER_NAME );
        }
    }
    return canBuildJob;
}

// CanBuildJob any (WorkerRecords)
//------------------------------------------------------------------------------
bool WorkerRecords::CanBuildJob( const bool includeLocal ) const
{
    bool canBuildJob = false;
    const size_t numExistingRecords = GetSize();
    for ( size_t i=0; i<numExistingRecords; ++i )
    {
        canBuildJob = CanBuildJob( i, includeLocal );
        if ( canBuildJob )
        {
            break;
        }
    }
    return canBuildJob;
}

//------------------------------------------------------------------------------
