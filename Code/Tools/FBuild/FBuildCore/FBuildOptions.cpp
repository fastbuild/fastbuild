// FBuild - the main application
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Tools/FBuild/FBuildCore/PrecompiledHeader.h"

// FBuildCore
#include "FBuildOptions.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Math/Murmur3.h"

// system
#if defined( __WINDOWS__ )
    #include <windows.h> // for QueryDosDeviceA
#endif

// CONSTRUCTOR - FBuildOptions
//------------------------------------------------------------------------------
FBuildOptions::FBuildOptions()
: m_ForceCleanBuild( false )
, m_UseCacheRead( false )
, m_UseCacheWrite( false )
, m_ShowInfo( false )
, m_ShowCommandLines( false )
, m_ShowErrors( true )
, m_ShowProgress( false )
, m_AllowDistributed( false )
, m_ShowSummary( false )
, m_SaveDBOnCompletion( false )
, m_GenerateReport( false )
, m_NoLocalConsumptionOfRemoteJobs( false )
, m_AllowLocalRace( true )
, m_WrapperChild( false )
, m_FixupErrorPaths( false )
, m_StopOnFirstError( true )
, m_WorkingDirHash( 0 )
{
#ifdef DEBUG
	//m_ShowInfo = true; // uncomment this to enable spam when debugging
#endif

	// Default to NUMBER_OF_PROCESSORS
	m_NumWorkerThreads = Env::GetNumProcessors();

	// Default working dir is the system working dir
    AStackString<> workingDir;
    VERIFY( FileIO::GetCurrentDir( workingDir ) );
    SetWorkingDir( workingDir );
}

// FBuildOptions::SetWorkingDir
//------------------------------------------------------------------------------
void FBuildOptions::SetWorkingDir( const AString & path )
{
    ASSERT( !path.IsEmpty() );
	m_WorkingDir = path;

	// clean path
	PathUtils::FixupFolderPath( m_WorkingDir );
        
    // no trailing slash
    if ( m_WorkingDir.EndsWith( NATIVE_SLASH ) )
    {
        m_WorkingDir.SetLength( m_WorkingDir.GetLength() - 1 );
    }

	#if defined( __WINDOWS__ )
		// so C:\ and c:\ are treated the same on Windows, for better cache hits
		// make the drive letter always uppercase
		if ( ( m_WorkingDir.GetLength() >= 2 ) &&
			 ( m_WorkingDir[ 1 ] == ':' ) &&
			 ( m_WorkingDir[ 0 ] >= 'a' ) &&
			 ( m_WorkingDir[ 0 ] <= 'z' ) )
		{
			m_WorkingDir[ 0 ] = ( 'A' + ( m_WorkingDir[ 0 ] - 'a' ) );
		}
	#endif

    // Generate Mutex/SharedMemory names
    #if defined( __WINDOWS__ ) || defined( __OSX__ )
        #if defined( __WINDOWS__ )
            // convert subst drive mappings to the read path
            // (so you can't compile from the real path and the subst path at the
            // same time which would cause problems)
            AStackString<> canonicalPath;
            if ( ( m_WorkingDir.GetLength() >= 2 ) &&
                 ( m_WorkingDir[ 1 ] == ':' ) &&
                 ( m_WorkingDir[ 0 ] >= 'A' ) &&
                 ( m_WorkingDir[ 0 ] <= 'Z' ) )
            {
                // get drive letter without slash
                AStackString<> driveLetter( m_WorkingDir.Get(), m_WorkingDir.Get() + 2 );

                // get real path for drive letter (could be the same, or a subst'd path)
                char actualPath[ MAX_PATH ];
                actualPath[ 0 ] = '\000';
                VERIFY( QueryDosDeviceA( driveLetter.Get(), actualPath, MAX_PATH ) );

                // if returned path is of format "\??\C:\Folder"...
                if ( AString::StrNCmp( actualPath, "\\??\\", 4 ) == 0 )
                {
                    // .. then it means the working dir is a subst folder
                    // trim the "\\??\\" and use the real path as a base
                    canonicalPath = &actualPath[ 4 ];
                    canonicalPath += ( m_WorkingDir.Get() + 2 ); // add everything after the subst drive letter
                }
                else
                {
                    // The path was already a real path (QueryDosDevice returns the volume only)
                    canonicalPath = m_WorkingDir;
                }
            }
            else
            {
                // a UNC or other funky path - just leave it as is
                canonicalPath = m_WorkingDir;
            }
        #elif defined( __OSX__ )
            AStackString<> canonicalPath( m_WorkingDir );
        #endif

        // case insensitive
        canonicalPath.ToLower();
    #elif defined( __LINUX__ )
        // case sensitive
        AStackString<> canonicalPath( m_WorkingDir );
    #endif

    m_WorkingDirHash = Murmur3::Calc32( canonicalPath );
    m_ProcessMutexName.Format( "Global\\FASTBuild-0x%08x", m_WorkingDirHash );
    m_FinalProcessMutexName.Format( "Global\\FASTBuild_Final-0x%08x", m_WorkingDirHash );
    m_SharedMemoryName.Format( "FASTBuildSharedMemory_%08x", m_WorkingDirHash );
}

//------------------------------------------------------------------------------
