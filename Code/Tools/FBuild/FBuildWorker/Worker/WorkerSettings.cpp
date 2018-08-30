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
#define SETTINGS_FILENAME ".settings"
#define FBUILDWORKER_SETTINGS_MIN_VERSION ( 1 )     // Oldest compatible version
#define FBUILDWORKER_SETTINGS_CURRENT_VERSION ( 2 ) // Current version

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerSettings::WorkerSettings()
    : m_Mode( WHEN_IDLE )
, m_NumCPUsToUse( 1 )
, m_StartMinimized( false )
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

// GetWorkerTags
//------------------------------------------------------------------------------
const Tags & WorkerSettings::GetWorkerTags() const
{
    if ( !m_WorkerTags.IsValid() )
    {
        const size_t numTags = m_BaseWorkerTagStrings.GetSize();
        for ( size_t i=0; i<numTags; ++i )
        {
            m_BaseWorkerTags.ParseAndAddTag( m_BaseWorkerTagStrings.Get( i ) );
        }
        // always set valid, even if empty container
        m_BaseWorkerTags.SetValid( true );
        m_WorkerTags = m_BaseWorkerTags;
    }
    return m_WorkerTags;
}

// ApplyWorkerTags
//------------------------------------------------------------------------------
void WorkerSettings::ApplyWorkerTags( const Tags & workerTags )
{
    // ensure m_BaseWorkerTags is populated, before we apply changes to it
    GetWorkerTags();

    Tags removedTags;  // pass empty container, since only adding
    m_BaseWorkerTags.ApplyChanges( removedTags, workerTags );
    m_WorkerTags = m_BaseWorkerTags;

    // update m_BaseWorkerTagStrings to match base changes
    m_BaseWorkerTags.ToStringArray( m_BaseWorkerTagStrings );
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
        f.Read( m_BaseWorkerTagStrings );
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
        ok &= f.Write( m_BaseWorkerTagStrings );

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
