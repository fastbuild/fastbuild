// WorkerSettings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Singleton.h"

// Forward Declarations
//------------------------------------------------------------------------------

// WorkerSettings
//------------------------------------------------------------------------------
class WorkerSettings : public Singleton< WorkerSettings >
{
public:
    explicit WorkerSettings();
    ~WorkerSettings();

    // Worker Mode
    enum Mode
    {
        DISABLED        = 0, // Don't work for anyone
        WHEN_IDLE       = 1, // Work for others when idle
        DEDICATED       = 2, // Work for others always
        PROPORTIONAL    = 3  // Work for others proportional to free CPU
    };
    inline Mode GetMode() const { return m_Mode; }
    void SetMode( Mode m );

    inline uint32_t GetIdleThresholdPercent() const { return m_IdleThresholdPercent; }
    void SetIdleThresholdPercent( uint32_t p );

    // CPU Usage limits
    inline uint32_t GetNumCPUsToUse() const { return m_NumCPUsToUse; }
    void SetNumCPUsToUse( uint32_t c );

    // Start minimized
    void SetStartMinimized( bool startMinimized );
    inline bool GetStartMinimzed() const { return m_StartMinimized; }

    // Time settings were last changed/written to disk
    uint64_t GetSettingsWriteTime() const { return m_SettingsWriteTime; }

    inline uint32_t GetMinimumFreeMemoryMiB() const { return m_MinimumFreeMemoryMiB; }
    void SetMinimumFreeMemoryMiB( uint32_t value );

    void Load();
    void Save();

private:
    Mode        m_Mode;
    uint32_t	m_IdleThresholdPercent;
    uint32_t    m_NumCPUsToUse;
    bool        m_StartMinimized;
    uint64_t    m_SettingsWriteTime;    // FileTime of settings when last changed/written to disk
    uint32_t    m_MinimumFreeMemoryMiB; // Minimum OS free memory including virtual memory to let worker do its work
};

//------------------------------------------------------------------------------
