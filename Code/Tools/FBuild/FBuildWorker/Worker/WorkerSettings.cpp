// WorkerSettings
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerSettings.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileStream.h"
#include "Core/Strings/AStackString.h"

// system
#if defined( __WINDOWS__ )
    #include <Windows.h>
#endif

// Other
//------------------------------------------------------------------------------
#define FBUILDWORKER_SETTINGS_MIN_VERSION ( 2 )     // Oldest compatible version
#define FBUILDWORKER_SETTINGS_CURRENT_VERSION ( 3 ) // Current version

#define FBUILDWORKER_SETTINGS_SUPPORT_DETAILED_CPU (3) // Support detailed CPU in version 3+

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::WorkerSettings()
    : m_NumCPUsToUseWhenIdle( 1 )
	, m_NumCPUsToUseDedicated( 0 )
    , m_StartMinimized( false )
{
    // half CPUs available to use by default
    uint32_t numCPUs = Env::GetNumProcessors();
    m_NumCPUsToUseWhenIdle = Math::Max< uint32_t >( 1, numCPUs / 2 );

    Load();

    // handle CPU downgrade
    m_NumCPUsToUseWhenIdle = Math::Min( Env::GetNumProcessors(), m_NumCPUsToUseWhenIdle );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::~WorkerSettings() = default;

// SetNumCPUsToUseWhenIdle
//------------------------------------------------------------------------------
void WorkerSettings::SetNumCPUsToUseWhenIdle( uint32_t c )
{
    m_NumCPUsToUseWhenIdle = c;
}

// SetNumCPUsToUseDedicated
//------------------------------------------------------------------------------
void WorkerSettings::SetNumCPUsToUseDedicated(uint32_t c)
{
	m_NumCPUsToUseDedicated = c;
}

// SetStartMinimized
//------------------------------------------------------------------------------
void WorkerSettings::SetStartMinimized( bool startMinimized )
{
    m_StartMinimized = startMinimized;
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
        f.Read( m_NumCPUsToUseWhenIdle );
		if (header[3] >= FBUILDWORKER_SETTINGS_SUPPORT_DETAILED_CPU)
		{
			f.Read(m_NumCPUsToUseDedicated);
		}
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
        ok &= f.Write( m_NumCPUsToUseWhenIdle );
		ok &= f.Write(m_NumCPUsToUseDedicated);
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
