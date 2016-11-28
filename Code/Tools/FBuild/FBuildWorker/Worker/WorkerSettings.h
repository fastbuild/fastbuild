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
        WHEN_IDLE       = 1, // Work when others when idle
        DEDICATED       = 2  // Work for others always
    };
    inline Mode GetMode() const { return m_Mode; }
    void SetMode( Mode m );

    // CPU Usage limits
    inline uint32_t GetNumCPUsToUse() const { return m_NumCPUsToUse; }
    void SetNumCPUsToUse( uint32_t c );

    // Start minimzed
    void SetStartMinimized( bool startMinimized );
    inline bool GetStartMinimzed() { return m_StartMinimized; }

    void Load();
    void Save();
private:
    Mode        m_Mode;
    uint32_t    m_NumCPUsToUse;
    bool        m_StartMinimized;
};

//------------------------------------------------------------------------------
