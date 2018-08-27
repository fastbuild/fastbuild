// WorkerSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerSettings.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileStream.h"

// system
#if defined( __WINDOWS__ )
    #include <Windows.h>
#endif

// Other
//------------------------------------------------------------------------------
#define FBUILDWORKER_SETTINGS_MIN_VERSION ( 1 )     // Oldest compatible version
#define FBUILDWORKER_SETTINGS_CURRENT_VERSION ( 2 ) // Current version

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::WorkerSettings()
: m_Mode( WorkerSettings::WHEN_IDLE )
, m_NumCPUsToUse( 1 )
, m_StartMinimized( false )
, m_SandboxEnabled( false )
{
    // half CPUs available to use by default
    uint32_t numCPUs = Env::GetNumProcessors();
    m_NumCPUsToUse = Math::Max< uint32_t >( 1, numCPUs / 2 );

    // handle CPU downgrade
    m_NumCPUsToUse = Math::Min( Env::GetNumProcessors(), m_NumCPUsToUse );

    // load the settings file
    Load();
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::~WorkerSettings()
{
}

// SetWorkMode
//------------------------------------------------------------------------------
void WorkerSettings::SetMode( const WorkerSettings::Mode m )
{
    m_Mode = m;
}

// SetNumCPUsToUse
//------------------------------------------------------------------------------
void WorkerSettings::SetNumCPUsToUse( const uint32_t c )
{
    m_NumCPUsToUse = c;
}

// SetStartMinimized
//------------------------------------------------------------------------------
void WorkerSettings::SetStartMinimized( const bool startMinimized )
{
    m_StartMinimized = startMinimized;
}

// SetSandboxEnabled
//------------------------------------------------------------------------------
void WorkerSettings::SetSandboxEnabled( const bool sandboxEnabled )
{
    m_SandboxEnabled = sandboxEnabled;
}

// SetSandboxExe
//------------------------------------------------------------------------------
void WorkerSettings::SetSandboxExe( const AString & path )
{
    m_SandboxExe = path;
}

// SetSandboxArgs
//------------------------------------------------------------------------------
void WorkerSettings::SetSandboxArgs( const AString & args )
{
    m_SandboxArgs = args;
}

// SetSandboxTmp
//------------------------------------------------------------------------------
void WorkerSettings::SetSandboxTmp( const AString & path )
{
    m_SandboxTmp = path;
}

// Load
//------------------------------------------------------------------------------
void WorkerSettings::Load()
{
    AStackString<> settingsPath;
    Env::GetExePath( settingsPath );
    settingsPath += ".settings";

    FileStream f;
    if ( f.Open( settingsPath.Get(), FileStream::READ_ONLY ) )
    {
        char header[ 4 ] = { 0 };
        f.Read( &header, 4 );
        if ( ( header[ 3 ] < FBUILDWORKER_SETTINGS_MIN_VERSION ) ||
             ( header[ 3 ] > FBUILDWORKER_SETTINGS_CURRENT_VERSION ) )
        {
            return; // version is too old, or newer, and cannot be read
        }

        // settings
        uint32_t mode;
        f.Read( mode );
        m_Mode = (Mode)mode;
        f.Read( m_NumCPUsToUse );
        f.Read( m_StartMinimized );
    }
}

// Save
//------------------------------------------------------------------------------
void WorkerSettings::Save()
{
    AStackString<> settingsPath;
    Env::GetExePath( settingsPath );
    settingsPath += ".settings";

    FileStream f;
    if ( f.Open( settingsPath.Get(), FileStream::WRITE_ONLY ) )
    {
        bool ok = true;

        // header
        ok &= ( f.Write( "FWS", 3 ) == 3 );
        ok &= ( f.Write( uint8_t( FBUILDWORKER_SETTINGS_CURRENT_VERSION ) ) == 1 );

        // settings
        ok &= f.Write( (uint32_t)m_Mode );
        ok &= f.Write( m_NumCPUsToUse );
        ok &= f.Write( m_StartMinimized );

        if ( ok )
        {
            return;
        }
    }

    #if defined( __WINDOWS__ )
        MessageBox( nullptr, "Failed to save settings.", "FBuildWorker", MB_OK );
    #elif defined( __APPLE__ )
        // TODO:MAC Implement ShowMessageBox
    #elif defined( __LINUX__ )
        // TODO:LINUX Implement ShowMessageBox
    #else
        #error Unknown Platform
    #endif
}

//------------------------------------------------------------------------------
