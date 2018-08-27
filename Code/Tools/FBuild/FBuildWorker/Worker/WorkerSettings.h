// WorkerSettings
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Strings/AStackString.h"
#include "Core/Strings/AString.h"
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

    inline bool GetSandboxEnabled() { return m_SandboxEnabled; }
    void SetSandboxEnabled( const bool sandboxEnabled );
    inline const AString & GetSandboxExe() const { return m_SandboxExe; }
    void SetSandboxExe( const AString & path );
    inline const AString & GetAbsSandboxExe() const;
    inline const AString & GetSandboxArgs() const { return m_SandboxArgs; }
    void SetSandboxArgs( const AString & args );
    inline const AString & GetSandboxTmp() const { return m_SandboxTmp; }
    inline const AString & GetObfuscatedSandboxTmp() const;
    void SetSandboxTmp( const AString & path );
    
    void Load();
    void Save();
private:
    Mode             m_Mode;
    uint32_t         m_NumCPUsToUse;
    bool             m_StartMinimized;
    bool             m_SandboxEnabled;
    AString          m_SandboxExe;
    AString          m_SandboxArgs;
    AString          m_SandboxTmp;

    mutable AString  m_AbsSandboxExe;
    mutable AString  m_ObfuscatedSandboxTmp;
};

inline const AString & WorkerSettings::GetAbsSandboxExe() const
{
    if ( m_SandboxEnabled && !m_SandboxExe.IsEmpty())
    {
        if ( m_AbsSandboxExe.IsEmpty() )
        {
            if ( PathUtils::IsFullPath( m_SandboxExe ) )
            {
                m_AbsSandboxExe = m_SandboxExe;
            }
            else
            {
                // can't use app dir here, since want exe and tmp paths to be consistent
                // for what dir we use for base when expanding relative paths
                AStackString<> workingDir;
                FileIO::GetCurrentDir( workingDir );
                PathUtils::CleanPath( workingDir, m_SandboxExe, m_AbsSandboxExe );
            }
        }
        else
        {
            // use cached m_AbsSandboxExe value
        }
    }
    else
    {
        m_AbsSandboxExe.Clear();
    }
    return m_AbsSandboxExe;
}

inline const AString & WorkerSettings::GetObfuscatedSandboxTmp() const
{
    if ( m_SandboxEnabled && !m_SandboxTmp.IsEmpty())
    {
        if ( m_ObfuscatedSandboxTmp.IsEmpty() )
        {
            AStackString<> workingDir;
            FileIO::GetCurrentDir( workingDir );
            PathUtils::GetObfuscatedSandboxTmp(
                m_SandboxEnabled,
                workingDir,
                m_SandboxTmp,
                m_ObfuscatedSandboxTmp );
        }
        else
        {
            // use cached m_ObfuscatedSandboxTmp value
        }
    }
    else
    {
        m_ObfuscatedSandboxTmp.Clear();
    }
    return m_ObfuscatedSandboxTmp;
}

//------------------------------------------------------------------------------
