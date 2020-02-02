// WorkerSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerSettings.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include "Core/Env/WindowsHeader.h"
#endif

// Other
//------------------------------------------------------------------------------
#define FBUILDWORKER_SETTINGS_MIN_VERSION ( 1 )     // Oldest compatible version
#define FBUILDWORKER_SETTINGS_CURRENT_VERSION ( 4 ) // Current version

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::WorkerSettings()
    : m_Mode( WHEN_IDLE )
    , m_IdleThresholdPercent( 20 )
    , m_NumCPUsToUse( 1 )
    , m_StartMinimized( false )
    , m_SettingsWriteTime( 0 )
    , m_MinimumFreeMemoryMiB( 1024 ) // 1 GiB
{
    // half CPUs available to use by default
    uint32_t numCPUs = Env::GetNumProcessors();
    m_NumCPUsToUse = Math::Max< uint32_t >( 1, numCPUs / 2 );

    Load();

    // handle CPU downgrade
    m_NumCPUsToUse = Math::Min( Env::GetNumProcessors(), m_NumCPUsToUse );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::~WorkerSettings() = default;

// SetMode
//------------------------------------------------------------------------------
void WorkerSettings::SetMode( Mode m )
{
    m_Mode = m;
}

// SetIdleThresholdPercent
//------------------------------------------------------------------------------
void WorkerSettings::SetIdleThresholdPercent( uint32_t p )
{
    m_IdleThresholdPercent = p;
}

// SetNumCPUsToUse
//------------------------------------------------------------------------------
void WorkerSettings::SetNumCPUsToUse( uint32_t c )
{
    m_NumCPUsToUse = c;
}

// SetStartMinimized
//------------------------------------------------------------------------------
void WorkerSettings::SetStartMinimized( bool startMinimized )
{
    m_StartMinimized = startMinimized;
}

// SetMinimumFreeMemoryMiB
//------------------------------------------------------------------------------
void WorkerSettings::SetMinimumFreeMemoryMiB( uint32_t value )
{
    m_MinimumFreeMemoryMiB = value;
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
        uint8_t header[ 4 ] = { 0 };
        f.Read( &header, 4 );

        const uint8_t settingsVersion = header[ 3 ];
        if ( ( settingsVersion < FBUILDWORKER_SETTINGS_MIN_VERSION ) ||
             ( settingsVersion > FBUILDWORKER_SETTINGS_CURRENT_VERSION ) )
        {
            return; // version is too old, or newer, and cannot be read
        }

        // settings
        uint32_t mode;
        f.Read( mode );
        m_Mode = (Mode)mode;
        if ( header[ 3 ] >= 4 )
        {
            f.Read( m_IdleThresholdPercent );
        }
        f.Read( m_NumCPUsToUse );
        f.Read( m_StartMinimized );

        f.Close();

        m_SettingsWriteTime = FileIO::GetFileLastWriteTime( settingsPath );
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
        ok &= ( f.Write( uint8_t( FBUILDWORKER_SETTINGS_CURRENT_VERSION ) ) );

        // settings
        ok &= f.Write( (uint32_t)m_Mode );
        ok &= f.Write( m_IdleThresholdPercent );
        ok &= f.Write( m_NumCPUsToUse );
        ok &= f.Write( m_StartMinimized );

        f.Close();

        if ( ok )
        {
            m_SettingsWriteTime = FileIO::GetFileLastWriteTime( settingsPath );
            return;
        }
    }

    Env::ShowMsgBox( "FBuildWorker", "Failed to save settings." );
}

//------------------------------------------------------------------------------
