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

    // CPU Usage limits
    inline uint32_t GetNumCPUsToUseWhenIdle() const { return m_NumCPUsToUseWhenIdle; }
	inline uint32_t GetNumCPUsToUseDedicated() const { return m_NumCPUsToUseDedicated; }
    void SetNumCPUsToUseWhenIdle( uint32_t c );
	void SetNumCPUsToUseDedicated(uint32_t c);

    // Start minimzed
    void SetStartMinimized( bool startMinimized );
    inline bool GetStartMinimzed() { return m_StartMinimized; }

    void Load();
    void Save();
private:
    uint32_t    m_NumCPUsToUseWhenIdle;
	uint32_t    m_NumCPUsToUseDedicated;
    bool        m_StartMinimized;
};

//------------------------------------------------------------------------------
